#[Ce qui doit changer, ce qu'on doit garder]

##Pipeline
on change:
    * tout.

On a un seul "renderer" : le _RenderThread_. Cet objet peut pousser des commandes _RenderCommand_ dans une queue _RenderQueue_. Une clé est calculée pour chaque commande en fonction des ressources qu'elle implique (material, ...) et en fin de frame, la queue est triée via ces clés. Il en résulte une série de draw commands minimisant le state change (optimisée) que le _RenderThread_ peut pousser vers le GPU en async, alors que la frame d'après est calculée.

Les _Layers_ sont des objets empilables dans une _LayerStack_ possédée par l'_Application_, dont la fonction run() ne fait qu'itérer les layers dans l'ordre de soumission en appelant leur fonction update(). Cette fonction update est responsable de la soumission des _DrawCommand_ à la _RenderQueue_.

Quand tous les layers ont été itérés, la queue est triée et dispatchée via RenderThread::flush(), et (à travers l'indirection _GraphicsContext_) on a un call vers swap_buffers().

##Shaders

on change:
    * _Shader_ est une interface du moteur de rendu, et masque des implémentations gfx-driver-dependent
    * un _Shader_ est une des ressources référencées par _Material_
    * 1 seul fichier par shader, utiliser une directive custom pour la segmentation
    * a priori, les variantes dégagent
    * un _Shader_ doit détecter et gérer ses uniformes de façon intelligente (en particulier les samplers pour lesquels des texture slots sont attribués)

on garde:
    * le système d'includes
    * le système de hot swap
    * peut-être les defines





#[11-08-19]
Les trois derniers jours j'ai travaillé intensément au refactor du logger du projet WCore. Un nouvel event system a aussi vu le jour, adapté d'une trouvaille en ligne, bien plus facile d'utilisation que l'ancien, type-safe et single-header.

##Logger
Features:
	* Thread-safe
	* ostream syntax
	* Event tracking
	* Multiple sinks with channel subscription system
    * Zero cost macro stripping

Les streams de la STL ne sont pas thread-safe: quand deux threads tentent d'écrire sur std::cout par exemple, il est possible que les messages apparaissent entrelassés sur le terminal, car le flux est en accès concurrent. Rendre un flux synchronisable demande pas mal d'architecture.

L'idée est de créer un unique flux par thread, et pour chaque flux de contrôler quand un iomanip std::endl est envoyé. Alors seulement, le message complet est poussé dans une queue en accès synchronisé (même principe qu'une render queue). La queue est vidée régulièrement.

Les deux éléments clés pour implémenter cette idée sont :
1) L'utilisation de variables thread_local, pour s'assurer que chaque thread a accès à un unique flux créé dans son thread local storage.
2) L'aggrégation par le flux d'un type dérivé de std::stringbuf qui surcharge int sync(), de sorte à pouvoir placer un appel quand le flux reçoit un std::endl. En effet, envoyer std::endl dans un std::ostream provoque un flush(), ce qui entraîne l'appel à la fonction sync() du buffer sous-jacent.

Voici un synopsis de ce que fait mon code :
```cpp
class LoggerStream: public std::ostream
{
private:
	class StringBuffer: public std::stringbuf
	{
	    StringBuffer(LoggerStream& parent): parent_(parent) {}
		virtual int sync() override
		{
			parent_.submit(str());
			str("");
			return 0;
		}

	private:
		LoggerStream& parent_;
	};

public:
	LoggerStream(LoggerThread& logger_thread);

private:
	// Send message and current state to logger thread
	void submit(const std::string& message)
	{
		logger_thread_.enqueue(message);
	}

	StringBuffer buffer_; 				// The buffer this stream writes to
	LoggerThread& logger_thread_;		// Reference to unique logger thread
	LoggerThread::LogStatement stmt_;	// Current state
};

static LoggerThread LOGGER_THREAD;
// Return a unique stream per invoker thread, using thread local storage
static inline LoggerStream& get_log()
{
    thread_local LoggerStream ls(LOGGER_THREAD);
    return ls;
}
```
Noter qu'en plus de celà, _LoggerStream_ doit posséder un état, afin d'enregistrer les informations supplémentaires au texte (canal, type de message, timestamp, sévérité, numéro de ligne, fichier). Ces paramètres sont passés en paramètres à la fonction get_log() qui les transmet au stream via sa fonction prepare(). Un ensemble de macros qui appèlent get_log() en sous-main me permet cependant d'écourter largement la syntaxe pour tous les cas d'utilisations que j'ai rencontrés avec WCore.

La classe _LoggerThread_ fonctionne véritablement sur le principe de _RenderThread_. Un état atomique et deux variables de condition implémentent une machine à état très simple qui tourne dans la boucle principale de la fonction run(). Quand _LoggerThread_ est dans l'état STATE_IDLE, un producer thread peut venir verrouiller un mutex et pousser un message dans la queue avant de le déverrouiller. Un autre producer thread devra alors attendre sur ce mutex avant de pouvoir pousser un message. Un appel à flush() engendre l'état STATE_FLUSH dans lequel la queue est triée par timestamp et vidée (on verra dans quoi juste après), et kill() provoque un flush et l'état STATE_KILLED dans lequel le thread va simplement join().

Alors, dans quoi ais-je bien pu vider ma queue ? J'ai écrit une interface _Sink_ qui représente une voie de traitement (et un end-point) pour les messages de la queue. Le _LoggerThread_ peut enregistrer plusieurs _Sink_ spécialisés (un pour la console, un pour un fichier log, un pour mon futur web-viewer...), qui souscrivent aux canaux qui les intéressent. Lors du flush(), quand un message passe le test de verbosité, il se retrouve propagé dans tous les sinks qui ont souscrit au canal sur lequel il a été émis. Chaque sink fait ensuite ce qu'il veut avec le message.
Des sinks spécialisés _ConsoleSink_ et _LogFileSink_ ont été écrits et testés.


Comment on s'en sert ?
logger_thread.h définit un _LoggerThread_ statique du nom de WLOGGER. Le logger est d'abord initialisé (création de canaux, ajout de sinks, event tracking...), puis le thread est lancé via spawn() :
```cpp
    WLOGGER.create_channel("TEST", 3);
    WLOGGER.create_channel("material", 2);
    WLOGGER.create_channel("accessibility", 2);
    WLOGGER.create_channel("render", 3);
    WLOGGER.create_channel("collision", 3);
    WLOGGER.attach_all("ConsoleSink", std::make_unique<dbg::ConsoleSink>());
    WLOGGER.attach("MainFileSink", std::make_unique<dbg::LogFileSink>("wcore.log"), {"core"_h, "material"_h, "render"_h, "collision"_h});
    WLOGGER.attach("EventFileSink", std::make_unique<dbg::LogFileSink>("events.log"), {"event"_h});
    WLOGGER.set_backtrace_on_error(true);
    WLOGGER.track_event<DamageEvent>();
    WLOGGER.track_event<ItemFoundEvent>();
    WLOGGER.spawn();

    BANG();
    DLOGN("render") << "Notify: Trump threatened Iran with \'obliteration\'." << std::endl;
    DLOGI           << "Item 1" << std::endl;
    DLOGI           << "Item 2" << std::endl;
    DLOGI           << "Item 3" << std::endl;
    DLOGW("core") << "Warning: Iran said \"Double dare you!\"" << std::endl;
    DLOGE("core") << "Error 404: Nuclear war heads could not be found." << std::endl;
    DLOGF("core") << "Fatal error: Index out of bounds in array: war_heads." << std::endl;

    WLOGGER.flush();
```

Le header logger.h définit les macros d'accès au flux (DLOG, DLOGI, DLOGW...). Cette séparation est jugée nécessaire, car un seul système a besoin de configurer, lancer et flusher le _LoggerThread_ (et doit donc inclure logger_thread.h), mais tout le reste du moteur peut se contenter d'inclure logger.h pour faire du logging, évitant d'avoir à inclure toute la bête à chaque fois.

L'ajout de couleurs se fait au moyen d'une structure _WCC_ qui est évaluée en séquence d'échappement ANSI lorsque passée à un flux ostream. Donc plus de parsing de balises lourdingue. J'ai conservé les mêmes mnémoniques pour les couleurs que dans WCore, mais aussi ajouté un constructeur généraliste. L'envoi d'un WCC(0) est supposé restaurer le style précédent, mais ne restaure que le style *par défaut* pour l'instant.
```cpp
    DLOG("core",1) << "This " << WCC('i') << "word" << WCC(0) << " is orange." << std::endl;

    for(int ii=0; ii<10; ++ii)
    {
        for(int jj=0; jj<10; ++jj)
            DLOG("core",1) << WCC(25*ii,25*jj,255-25*jj) << char('A'+ii+jj) << " ";
        DLOG("core",1) << std::endl;
    }
    WLOGGER.flush();
```

Le fichier tests/test_logger.h implémente une fonction test complète, dont un test de concurrence avec plusieurs worker threads qui poussent plein de messages en même temps. Le résultat est impeccable, si je peux me permettre.


###Disable logging
Le define LOGGING_ENABLED permet magiquement de supprimer toutes les instructions de log quand initialisé à 0. Ceci est possible grâce à une conditionnelle constexpr présente dans chaque macro DLOGx :

```cpp
    #define DLOG(C,S) if constexpr(!LOGGING_ENABLED); else get_log( H_( (C) ), dbg::MsgType::NORMAL, (S) )
```
Ceci est inspiré de [2]-(deuxième meilleure réponse). Si LOGGING_ENABLED est à 0, l'expression 
```cpp
    DLOG("core",1) << "Hello " << 1+1 << std::endl;
```
est l'instruction vide :
```cpp
    if constexpr(true);
```
Et les arguments ne sont même pas évalués (ce qui est bien plus intéressant du point de vue des performances que d'utiliser un Null Stream). Sinon la macro évalue à :
```cpp
    if constexpr(false); else get_log("core"_h, ...) << "Hello " << 1+1 << std::endl;
```
Ce qui est immédiatement optimisé par le compilateur en 
```cpp
    get_log("core"_h, ...) << "Hello " << 1+1 << std::endl;
```

La macro WLOGGER est similaire, et le logging thread n'est instancié que si LOGGING_ENABLED est à 1. En définitive, on peut faire complètement disparaître toutes les macros de logging en changeant une valeur à la compilation, en mode zero cost. On peut imaginer proposer au client deux versions binaires de la lib : une avec logging et une sans, et la question d'activer le logging devient un simple choix de lib dynamique.

###Sources:
	[1] https://www.techrepublic.com/article/use-stl-streams-for-easy-c-plus-plus-thread-safe-logging/#
    [2] https://stackoverflow.com/questions/11826554/standard-no-op-output-stream

###TODO:
	[X] Pimpl the shit out of the interface
	[ ] Write _NetSink_


#[07-09-19]
##Entry point
WCore séparait très mal le code client du code moteur, toute mon API était une vaste blague et le concept même d'application level était en suspens. J'ai adpoté pour ce projet une approche à la TheCherno sur son moteur Hazel. Le client hérite d'une classe _Application_ et la spécialise pour ses besoin, puis définit une fonction erwin::create_application() exportée par la lib qui retourne le type dérivé. C'est la lib qui génère la fonction main, crée l'application au moyen de la fonction create_application(), la fait tourner puis la détruit.

La classe _Application_ est déclarée dans application.h ainsi que la fonction create_application() :
```cpp
namespace erwin
{

class W_API Application
{
public:
    Application();
    virtual ~Application();

    void run();

private:
    bool is_running_;
};

// Defined in the client
Application* create_application();

} // namespace erwin
```
Noter l'attribut macro W_API qui sous Windows (W_PLATFORM_WINDOWS defined) évalue à 
```cpp
__declspec(dllexport)
// ou bien
__declspec(dllimport)
```
selon que W_BUILD_LIB est définit ou non (W_BUILD_LIB définit lors de la compilation de la liberwin, mais pas lors de la compilation du code client). Sous Linux, W_API est une macro vide. Voir core.h :
```cpp
#ifdef W_PLATFORM_WINDOWS
    #ifdef W_BUILD_LIB
        #define W_API __declspec(dllexport)
    #else
        #define W_API __declspec(dllimport)
    #endif
#else
    #define W_API
#endif
```

Le header entry_point.h est simplement :
```cpp
extern erwin::Application* erwin::create_application();

int main(int argc, char** argv)
{
    auto app = erwin::create_application();
    app->run();
    delete app;
}
```

Côté client on n'a qu'à inclure l'unique header erwin.h (qui regroupe toutes les inclusions core/event pertinentes de l'API), à spécialiser _Application_ et à définir create_application() :

```cpp
#include "erwin.h"

class Sandbox: public erwin::Application
{
public:
    Sandbox(){ }
    ~Sandbox(){ }
};

erwin::Application* erwin::create_application()
{
    return new Sandbox();
}
```
Et hop, notre application est maintenant gérée par la lib. Il reste bien entendu à définir du contenu fonctionnel pour l'application : un système de Layers et la gestion des events.

Pour l'instant, la fonction run() configure et spawn le logger thread, puis lance une boucle vide cadencée à 60fps (le logger est flushé en fin de boucle). Plus tard, chaque layer sera updaté itérativement dans cette boucle.


##glfw submodule
>> cd source/vendor
>> git submodule add https://github.com/glfw/glfw.git glfw
>> sudo apt-get install xorg-dev libxrandr-dev libxinerama-dev libxcursor-dev 
>> cd glfw;mkdir build;cd build
>> cmake ..
>> make
>> cp src/libglfw3.a ../../../../lib/
