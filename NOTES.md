#[General]
##Liste des extensions OpenGL nécessaires
    * GL_ARB_gl_spirv
    * GL_EXT_framebuffer_sRGB
    * GL_EXT_texture_compression_s3tc
    * GL_EXT_texture_filter_anisotropic
    * GL_EXT_texture_sRGB
    * GL_KHR_texture_compression_astc_hdr
    * GL_KHR_texture_compression_astc_ldr

##[Command Galore]
###Callgrind profiling
> valgrind --tool=callgrind ../bin/sandbox
> gprof2dot --format=callgrind -s --skew=0.1 ./callgrind.out.XXXXX | dot -Tsvg -o callgrind.svg
> gprof2dot --format=callgrind -zerwin::* -lerwin::* -n0.1 -s --skew=0.1 ./callgrind.out.XXXXX | dot -Tsvg -o callgrind.svg

###Apitrace usage
> apitrace trace --api=gl --output=sandbox.trace ../bin/sandbox
> LD_LIBRARY_PATH=/home/ndx/Qt/5.13.0/gcc_64/lib qapitrace sandbox.trace

##[GIT]
###Workflow
On veut conserver une copie fonctionnelle sur la branche master à tout instant. Pour introduire un nouveau feature, on va passer sur la branche de développement, créer une autre branche spécifiquement pour ce feature, faire les modifications/ajouts nécessaires, puis commit... Quand le feature est complètement testé et digne d'être intégré dans la branche master, on merge. Typiquement, la branche "master" est associée au *production server*, et la branche "dev" au *staging server*.

Créer une branche nommée "develop" :
> git branch develop

Afficher l'ensemble des branches ainsi que la branche active :
> git branch
      develop
    * master

Passer sur la branche develop :
> git checkout develop

Créer une nouvelle branche (qui référence la branche develop) pour le feature et passer dessus automatiquement :
> git checkout -b feature/new-feature-name

On implémente le feature, on teste massivement, on commit, on itère... Jusqu'au commit final pour ce feature :
> git commit -am "Done implementing new feature"

Noter qu'avec l'argument "a" on "git add" automatiquement tous les fichiers. On repasse maintenant sur la branche develop :
> git checkout develop

A ce stade, la branche develop ne contient pas le nouveau feature. On va merge avec la branche "feature/new-feature-name", puis on va détruire la feature branch :
> git merge feature/new-feature-name
> git branch -d feature/new-feature-name
> git push origin --delete feature/new-feature-name

Git vient de créer un "merge commit". Avec "git log" on se rend compte que le dernier commit sur la branche feature/new-feature-name vient d'être ajouté à la branche develop.

###git-flow
Pour se simplifier le travail, on peut utiliser le plugin git-flow :
> sudo apt-get install git-flow

Il faut initialiser git-flow sur un repo git déjà initialisé :
> git flow init -d


##TODO:
    [ ] Générer les mipmaps d'atlas manuellement (pour éviter le bleeding). Voir :
    https://computergraphics.stackexchange.com/questions/4793/how-can-i-generate-mipmaps-manually
        [ ] Cela suppose pour chaque asset de fabriquer les mipmaps dans Fudge et de toutes les
        stocker dans les CAT files.
    [X] Supporter SPIR-V
    https://www.khronos.org/opengl/wiki/SPIR-V
    https://www.khronos.org/opengl/wiki/SPIR-V/Compilation
    https://eleni.mutantstargoat.com/hikiko/2018/03/04/opengl-spirv/
    [/] Ecrire un renderer 2D multi-threaded
        [ ] Ecrire des classes de GUI basiques tirant parti du renderer 2D
    [ ] Ecrire un renderer 3D multi-threaded
    [X] Ecrire un script de building pour tout le projet (gère les deps...)
    [ ] Gérer le callback d'erreurs OpenGL
    https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDebugMessageCallback.xhtml 

###GO MT - ROADMAP:
    [X] Command queues
    [X] Master renderer
    [ ] Gfx resources manag.

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
    WLOGGER.attach("MainFileSink", std::make_unique<dbg::LogFileSink>("wcore.log"), 
        {"core"_h, "material"_h, "render"_h, "collision"_h});
    WLOGGER.attach("EventFileSink", std::make_unique<dbg::LogFileSink>("events.log"), 
        {"event"_h});
    WLOGGER.set_backtrace_on_error(true);
    WLOGGER.track_event<DamageEvent>();
    WLOGGER.track_event<ItemFoundEvent>();
    WLOGGER.spawn();

    BANG();
    DLOGN("render") << "Notify: Trump threatened Iran with \'obliteration\'." 
                    << std::endl;
    DLOGI           << "Item 1" << std::endl;
    DLOGI           << "Item 2" << std::endl;
    DLOGI           << "Item 3" << std::endl;
    DLOGW("core") << "Warning: Iran said \"Double dare you!\"" << std::endl;
    DLOGE("core") << "Error 404: Nuclear war heads could not be found." 
                  << std::endl;
    DLOGF("core") << "Fatal error: Index out of bounds in array: war_heads." 
                  << std::endl;

    WLOGGER.flush();
```

Le header logger.h définit les macros d'accès au flux (DLOG, DLOGI, DLOGW...). Cette séparation est jugée nécessaire, car un seul système a besoin de configurer, lancer et flusher le _LoggerThread_ (et doit donc inclure logger_thread.h), mais tout le reste du moteur peut se contenter d'inclure logger.h pour faire du logging, évitant d'avoir à inclure toute la bête à chaque fois.

L'ajout de couleurs se fait au moyen d'une structure _WCC_ qui est évaluée en séquence d'échappement ANSI lorsque passée à un flux ostream. Donc plus de parsing de balises lourdingue. J'ai conservé les mêmes mnémoniques pour les couleurs que dans WCore, mais aussi ajouté un constructeur généraliste. L'envoi d'un WCC(0) est supposé restaurer le style précédent, mais ne restaure que le style *par défaut* pour l'instant.
```cpp
    DLOG("core",1) << "This " << WCC('i') << "word" << WCC(0) 
                   << " is orange." << std::endl;

    for(int ii=0; ii<10; ++ii)
    {
        for(int jj=0; jj<10; ++jj)
            DLOG("core",1) << WCC(25*ii,25*jj,255-25*jj) << char('A'+ii+jj) << " ";
        DLOG("core",1) << std::endl;
    }
    WLOGGER.flush();
```

Le fichier tests/test_logger.h implémente une fonction test complète, dont un test de concurrence avec plusieurs worker threads qui poussent plein de messages en même temps. Le résultat est impeccable, si je peux me permettre.

![Exemple d'output du logger.\label{figLogger}](../../Erwin_rel/screens_erwin/erwin_0d_logger.png)

###Disable logging
Le define LOGGING_ENABLED permet magiquement de supprimer toutes les instructions de log quand initialisé à 0. Ceci est possible grâce à une conditionnelle constexpr présente dans chaque macro DLOGx :

```cpp
    #define DLOG(C,S) if constexpr(!LOGGING_ENABLED); else get_log( H_( (C) ), \
                                                  dbg::MsgType::NORMAL, (S) )
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
	[1] https://www.techrepublic.com/article/use-stl-streams-for-easy-c
        -plus-plus-thread-safe-logging/#
    [2] https://stackoverflow.com/questions/11826554/standard-no-op-output-stream

###TODO:
	[X] Pimpl the shit out of the interface
	[ ] Write _NetSink_


#[07-09-19]
##Entry point
WCore séparait très mal le code client du code moteur, toute mon API était une vaste blague et le concept même d'application level était en suspens. J'ai adpoté pour ce projet une approche à la TheCherno sur son moteur Hazel. Le client hérite d'une classe _Application_ et la spécialise pour ses besoin, puis définit une fonction erwin::create_application() exportée par la lib qui retourne le type dérivé. C'est la lib qui génère la fonction main(), crée l'application au moyen de la fonction create_application(), la fait tourner puis la détruit.

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
selon que W_BUILD_LIB est défini ou non (W_BUILD_LIB définit lors de la compilation de la liberwin, mais pas lors de la compilation du code client). Sous Linux, W_API est une macro vide. Voir core.h :
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
#include "core/application.h"

namespace erwin
{
    extern Application* create_application();
}

int main(int argc, char** argv)
{
    auto app = erwin::create_application();
    app->run();
    delete app;
}
```

Dans le header erwin.h (qui regroupe toutes les inclusions core/event pertinentes de l'API) le point d'entrée n'est inclu que si la macro *W_ENTRY_POINT* est définie :
```cpp
#ifdef W_ENTRY_POINT
    #include "core/entry_point.h"
#endif
```
Ce mécanisme permet de ne pas inclure plusieurs fois le point d'entrée dans le code client, ce qui définirait main() autant de fois et créerait un bug à la compilation. Seul le cpp qui surcharge _Application_ est supposé définir cette macro: pour créer une nouvelle application sous Erwin engine, on définit *W_ENTRY_POINT*, on inclut l'unique header erwin.h, on spécialise _Application_ et on définit create_application() :

```cpp
#define W_ENTRY_POINT
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


#[09-09-19]
##GLFW & event callbacks
J'ai cette fois intégré GLFW3 en tant que sous-module git :
> cd source/vendor
> git submodule add https://github.com/glfw/glfw.git glfw
> sudo apt-get install xorg-dev libxrandr-dev libxinerama-dev libxcursor-dev 
> cd glfw;mkdir build;cd build
> cmake ..
> make
> cp src/libglfw3.a ../../../../lib/

GLFW ne sera vraisemblablement utilisé qu'avec OpenGL (ne fonctionne pas avec DX à ma connaissance), et j'ai donc créé une abstraction pour la gestion de la fenêtre. L'interface core _Window_ permet de masquer l'implémentation sous-jacente. _GLFWWindow_ hérite de cette interface et définit la fonction statique create() déclarée par _Window_ pour retourner le type dérivé. A la construction, GLFW est initialisé ainsi que les propriétés de la fenêtre.

Contrairement à ce que je faisais dans WCore, j'utilise massivement les callbacks de GLFW pour propager mes événements custom dans le moteur. Chaque callback publie un événement sur l'_EventBus_, et modifie éventuellement l'état de la fenêtre au moyen d'un user pointer (un pour chaque fenêtre) que GLFW nous laisse préciser :

```cpp
    glfwSetWindowUserPointer(data_->window, &(*data_));
    // ...
    // Window resize event
    glfwSetWindowSizeCallback(data_->window, [](GLFWwindow* window, int width, int height)
    {
        auto* data = static_cast<GLFWWindowDataImpl*>(glfwGetWindowUserPointer(window));
        data->width = width;
        data->height = height;
        EVENTBUS.publish(WindowResizeEvent(width, height));
    });
```
Ici, data_ est une structure pimpl de type _GLFWWindowDataImpl_ qui possède un pointeur sur la fenêtre ainsi que ses variables d'état (taille, vsync...).

Pour l'instant, j'intercepte les événements souris (clic, scroll et mouvement), clavier, redimensionnement de la fenêtre et fermeture de la fenêtre. Tous ces événements sont définis dans event/window_events.h.


##Layers & LayerStack
Comme je choisis une approche de type "layered rendering" pour ce moteur, j'ai codé de nouvelles classes. Un _Layer_ représente une couche graphique (ou purement abstraite), activable/désactivable, qui peut réagir à des événements. Tous les layers sont possédés par un conteneur spécialisé : la _LayerStack_. On distingue un type particulier de layers : les overlays, qui sont toujours ajoutés en fin de stack.

La _LayerStack_ est possédée par la classe _Application_ qui définit des méthodes pour ajouter des layers/overlays. A chaque frame, la boucle principale itère sur tous les _Layers_ contenus dans la _LayerStack_ et exécute leurs fonctions update(), dans l'ordre croissant des indices :

```cpp
    while(is_running_)
    {
        // For each layer, update
        for(auto* layer: layer_stack_)
            layer->update();

        // ...
        window_->update();
    }
```
Les événements en revanche doivent être propagés dans le sens inverse. En effet, le dernier layer de la stack est celui qui est affiché en dernier, et par conséquent au dessus de tous les autres. Si ce dernier layer est par exemple un GUI d'inventaire avec un bouton, un clic sur le bouton ne doit pas être propagé aux layers du dessous. Si c'était le cas, un clic sur le bouton de l'inventaire pourrait faire tirer le gros flingue du joueur...

L'ennui est que mon _EventBus_ propage les événements dans l'ordre de souscription, ce qui n'est pas adapté dans le cas présent. J'ai rectifié le tir en faisant de _LayerStack_ un subscriber, et je le laisse gérer le dispatching dans sa fonction de handling :

```cpp
    template <typename EventT>
    void track_event()
    {
        EVENTBUS.subscribe(this, &LayerStack::dispatch<EventT>);
    }

    template <typename EventT>
    bool dispatch(const EventT& event)
    {
        bool handled = false;
        for(auto it=layers_.end(); it!=layers_.begin();)
        {
            Layer* layer = *--it;
            if(!layer->is_enabled()) continue;
            if(layer->on_event(event))
            {
                handled = true;
                break;
            }
        }

        return handled;
    }
```
La fonction template track_event() va (comme chez le _LoggerThread_) enregistrer une fonction de handling (template aussi) du nom de dispatch(). dispatch() va parcourir les layers en sens inverse et appeler les fonctions pertinentes de l'overload set on_event(). La classe _Layer_ va en effet définir plusieurs fonctions du nom de on_event() avec un type d'argument par événement géré. Comme mon système ne permet pas de propager des événements sous la forme de la classe de base _WEvent_, c'est le meilleur moyen que j'ai trouvé.
L'approche "casting" de TheCherno pour son event system m'a pour le coup semblé trop boilerplate intensive et peu flexible : mon système laisse le client définir des événements customs s'il le souhaite, sans besoin d'altérer d'éventuelles enums nécessaires au transtypage depuis/vers un type de base...

Pour qu'un layer puisse gérer un événement de type _SomeEvent_ il faut donc deux choses :
    * Appeler la fonction LayerStack::track_event<SomeEvent>() dans le constructeur de _Application_.
    * Définir une fonction virtuelle Layer::on_event(const SomeEvent&).
Les layers spécialisés héritant de la classe _Layer_ peuvent ou non surcharger les fonctions on_event() au gré de leurs besoins :

```cpp
class TestLayer: public Layer
{
public:
    virtual bool on_event(const MouseButtonEvent& event) override
    {
        DLOGN("event") << get_name() << " -> Handled event: " << event << std::endl;
        return true;
    }

protected:
    virtual void on_update() override { /* ... */ }
};
```

Noter qu'au sein de la classe _Layer_, on peut utiliser la macro REACT(SomeEvent) pour définir automatiquement une telle fonction virtuelle avec un comportement par défaut :

```cpp
#define REACT(EVENT) virtual bool on_event(const EVENT& event) { return false; }

class Layer
{
public:
    // ...
    REACT(KeyboardEvent)
    REACT(MouseButtonEvent)
    REACT(MouseScrollEvent)
    REACT(WindowResizeEvent)
    // ...
};
```
Je pense pouvoir me satisfaire de cette approche dans la mesure où mes layers n'ont vraisemblablement besoin de réagir qu'à un nombre très restreint de types d'événements.

##ImGui Docking
J'ai ajouté ImGui en git submodule. Pour profiter des features expérimentaux de la branche "docking" je dois changer le tag :
> cd source/vendor
> git submodule add https://github.com/ocornut/imgui.git imgui
> cd imgui
> git status
    On branch master
> git checkout docking
> cd ..
> git add imgui

NOTE : A l'avenir, pour éviter des galères quand le programme segfault à cause de ImGui, penser à bien tout compiler en mode debug, les assertions d'ImGui sont un bon outil de diagnostique.

La classe _ImGuiLayer_ qui hérite de l'interface _Layer_, est un overlay spécial de l'application possédant les fonctions ImGuiLayer::begin() et ImGuiLayer::end() qui encadrent le rendu de widgets dans la main loop. Chaque layer peut surcharger une fonction Layer::on_imgui_render() qui sera appelée entre begin() et end() dans la main loop :

```cpp
    while(is_running_)
    {
        // For each layer, update
        for(auto* layer: layer_stack_)
            layer->update();

        // TODO: move this to render thread when we have one
        IMGUI_LAYER->begin();
        for(auto* layer: layer_stack_)
            layer->on_imgui_render();
        IMGUI_LAYER->end();

        // ...
    }
``` 
Ainsi, chaque layer peut définir un petit widget de debug facilement (à la manière des GameSystem::generate_widget() de WCore) :

```cpp
class TestLayer: public Layer
{
public:
    // ...
    virtual void on_imgui_render() override
    {
        ImGui::Begin("Test Widget");
        // ... ImGui code ...
        ImGui::End();
    }
    // ...
}
```

_ImGuiLayer_ surcharge quelques fonctions on_event() de sorte à stopper la propagation des événements à travers les layers quand ImGui les a déjà dispatchés :
```cpp
bool ImGuiLayer::on_event(const KeyboardEvent& event)
{
    // Don't propagate event if ImGui wants to handle it
    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureKeyboard;
}
```

#[11-09-19]
##Asserts
La macro W_ASSERT de core/core.h permet conditionnellement d'afficher un message d'erreur et de poser un breakpoint pour faciliter le debugging :

```cpp
// Sous linux :
#define W_ASSERT(CND, ...) { if(!(CND)) { printf("Assertion Failed: %s\n", __VA_ARGS__); \
                                          raise(SIGTRAP); } }
// Sous windows
#define W_ASSERT(CND, ...) { if(!(CND)) { printf("Assertion Failed: %s\n", __VA_ARGS__); \
                                          __debugbreak(); } }
// Avec :
#define W_STATIC_ERROR(FSTR, ...) printf( FSTR , __VA_ARGS__ )
```

##Render device
J'ai une interface abstraite pour l'API graphique : _RenderDevice_. L'implémentation _OGLRenderDevice_ est spécifique à OpenGL. La classe statique _Gfx_ permet de sélectionner l'implémentation sous-jacente via Gfx::set_api() et un type énuméré, ou récupérer l'implémentation courante via Gfx::get_api(). Ce mécanisme permet aux factory methods create() des classes graphiques abstraites de renvoyer le type pertinent. Par exemple, l'interface _VertexBuffer_ possède une fonction statique VertexBuffer::create() qui renvoie un VertexBuffer* upcasté depuis un _OGLVertexBuffer_ si l'API courante est OpenGL.
Les fonctions élémentaires du _RenderDevice_ sont accessibles depuis un pointeur unique de la classe _Gfx_, et plusieurs types énumérés subsument ceux de l'API graphique :

```cpp
    Gfx::device->set_clear_color(0.2f, 0.2f, 0.2f, 1.0f);
    Gfx::device->clear(ClearFlags::CLEAR_COLOR_FLAG | ClearFlags::CLEAR_DEPTH_FLAG);
    Gfx::device->set_cull_mode(CullMode::Back);
    // ...
```

##Buffers
J'ai des couples interface/implémentation OpenGL pour les vertex buffers, les index buffers et les vertex arrays dans le header "render/buffer.h", reprenant l'abstraction déjà commencée dans WCore. Leur utilisation est aisée :

```cpp
    BufferLayout vertex_color_layout =
    {
        {"a_position"_h, ShaderDataType::Vec3},
        {"a_color"_h,    ShaderDataType::Vec3},
    };

    float* vertex_data = // ...
    uint32_t* index_data = // ...

    auto vb = std::shared_ptr<VertexBuffer>(VertexBuffer::create(vertex_data, 
                                            vertex_data_size, vertex_color_layout));

    auto ib = std::shared_ptr<IndexBuffer>(IndexBuffer::create(index_data, index_data_size));

    auto va = std::shared_ptr<VertexArray>(VertexArray::create());
    va->set_index_buffer(ib);
    va->add_vertex_buffer(vb);
```

##Shaders
J'ai un couple interface/implémentation _Shader_/_OGLShader_ pour les shaders. Deux factory methods permettent la génération d'une instance de shader depuis soit un flux std::istream, soit une string contenant la source.
J'ai bien écrit LA source. D'expérience, je ne gagne rien à séparer les sources des vertex / geometry / fragment / ... shaders, donc maintenant, tous sont écrits dans le même fichiers avec des balises de section #type :

```glsl
    #type vertex
    #version 410 core

    layout(location = 0) in vec3 in_position;
    layout(location = 1) in vec3 in_color;

    out vec3 v_color;

    void main()
    {
        gl_Position = vec4(in_position, 1.f);
        v_color = in_color;
    }

    #type fragment
    #version 410 core

    in vec3 v_color;
    layout(location = 0) out vec4 out_color;

    void main()
    {
        out_color = vec4(v_color,1.0f);
    }
```
L'implémentation gère le parsing de cette unique source en produisant un vecteur de paires <ShaderType, source>, lequel est itéré pour compiler tour à tour chaque shader du programme. Ensuite le programme est linké et le registre des uniformes est créé. J'ai repris le code d'error reporting de WCore et amélioré l'aspect logging :

    [0.212466][sha]  ⁕  Building OpenGL Shader program: "test_shader" 
    [0.212476][sha]      ↳ Compiling Vertex shader.
    [0.212525][sha]      ↳ Compiling Fragment shader.
    [0.212536][sha]      ↳ Linking program.
    [0.212733][sha]      ↳ Program "test_shader" is ready.
    [0.212740][sha]  ⁕  Detected 2 active attributes:
    [0.212743][sha]      ↳ GL_FLOAT_VEC3 in_color loc= 1
    [0.212748][sha]      ↳ GL_FLOAT_VEC3 in_position loc= 0

L'envoi d'uniformes est défini au niveau de l'implémentation, via des méthodes templates, comme dans WCore.

En poursuivant l'exemple précédent, pour afficher le "contenu" du vertex array on ferait :
```cpp
    auto shader = Shader::create("test_shader", shader_source);
    // Dans la fonction update
    Gfx::device->bind_default_frame_buffer();
    shader->bind();
    Gfx::device->draw_indexed(va);
```

J'affiche ainsi mon premier triangle coloré depuis du code client !


TODO:
    [X] Gérer les includes.
    [ ] Gérer le hotswap / reload
        -> Plutôt depuis la _ShaderBank_ quand celle-ci sera construite


##QueryTimer
J'ai repris le bon vieux GPU query timer de WCore, et lui ai fait une interface API-agnostic avec une factory method. Son type de sortie est un std::chrono::duration pour plus de rigueur.

```cpp
    QueryTimer* GPU_timer = QueryTimer::create();
    GPU_timer.start();
    // ... code to profile ...
    auto duration = GPU_timer.stop();
    std::cout << std::chrono::microseconds(duration).count() << std::endl;
```

##Intern strings
Le _InternStringLocator_ de WCore est réhabilité, ainsi que l'utilitaire de parsing des sources. J'ai viré toutes les dépendances à TinyXML, l'output est un simple fichier .txt clé/valeur parsé par le _InternStringLocator_.


#[15-09-19]
En visionnant un VLOG de Cherno qui montre une de ses journées de travail sur un renderer 2D (dont il ne donnait que peu de détails sur le fonctionnement interne, ce n'était pas le but de la vidéo), j'ai été super motivé à l'idée de coder un renderer 2D ultra-rapide moi-aussi. J'en ai donc codé deux.

Les deux renderers utilisent des stratégies différentes de batching pour diminuer au maximum le nombre de draw calls, mais leur fonctionnement dynamique est semblable. Je les ai donc regroupés derrière une même classe de base _Renderer2D_. Toutes les render requests doivent être placées entre Renderer2D::begin_scene() et Renderer2D::end_scene().
Le batching consiste à conserver les données par instance dans des buffers, et de soumettre ces données en fin d'envoi (end_scene()), plutôt que d'effectuer un draw call à chaque requête.
Le nombre de batches est géré dynamiquement, en particulier la suppression de batches inutilisés est effectuée de manière "intelligente", en attribuant un TTL à chaque batch. Les batches actifs ont toujours un TTL de 0, chaque batch inactif voit son TTL incrémenté à chaque frame. L'indice du premier batch à dépasser un TTL maximum est utilisé pour supprimer tous les batches à partir de cet indice. Ainsi un batch inactif est conservé un certain temps avant d'être supprimé, au cas où on en aurait besoin la frame d'après, évitant ainsi de supprimer et recréer des batches intempestivement si le nombre de draw requests oscille autour d'un multiple de la taille maximale d'un batch.

##Shader Storage Buffer Objects
Erwin engine supporte maintenant les SSBOs, buffers similaires aux Uniform Buffer Objects mais en read/write et sans la contrainte de taille débile à 64kB. Un SSBO peut être assez énorme.

En OpenGL, pour générer un SSBO on fait :
```cpp
    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data​, GLenum usage);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding_point, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind
```
"binding_point" est un nombre fixé à l'avance, et "data" est typiquement un array de structs :
```cpp
    struct InstanceData
    {
        glm::vec2 offset;
        glm::vec2 scale;
        glm::vec4 color;
    };
    std::vector<InstanceData> data;
```

Puis pour le lier à un shader :
```cpp
    GLuint block_index = glGetProgramResourceIndex(program_id, GL_SHADER_STORAGE_BLOCK, 
                                                   layout_name);
    glShaderStorageBlockBinding(program_id, block_index, binding_point);

```
La deuxième ligne a pour effet de modifier le binding point. Côté shader on a par exemple :
```glsl
struct InstanceData
{
    vec2 offset;
    vec2 scale;
    vec4 color;
};

layout(std430) buffer instance_data
{
    InstanceData inst[];
};

void main()
{
    vec3 offset = vec3(inst[gl_InstanceID].offset, 0.f);
    vec3 scale  = vec3(inst[gl_InstanceID].scale, 1.f);
    // ...
}
```

J'ai codé une interface API-agnostic _ShaderStorageBuffer_ et une implémentation OpenGL _OGLShaderStorageBuffer_, même délire que n'importe quel buffer dans buffer.h. La classe _Shader_ possède une méthode virtuelle attach_shader_storage() qui permet de lier un buffer donné au shader :
```cpp
    auto ssbo = std::shared_ptr<ShaderStorageBuffer>(ShaderStorageBuffer::create(binding_point, 
                                     nullptr, count, sizeof(InstanceData), DrawMode::Dynamic));
    // ...
    ssbo->map(data.data(), data.size());
    // ...
    shader.attach_shader_storage(ssbo, "instance_data");
```

##2D Batch Renderer
Le premier, _BatchRenderer2D_ possède une méthode draw_quad(3) qui prend en argument une position, une échelle et une couleur. Cette méthode va pousser le triangle inférieur droit du quad dans un vector de vertices (on verra pourquoi juste ce triangle suffit). Quand ce vector atteint une taille spécifique après plusieurs appels à draw_quad, il est entièrement copié dans un vertex buffer via la fonction map(). Si un autre quad doit être créé alors que le vertex buffer est plein, un autre vertex buffer est créé dynamiquement et il recevra le contenu du vector de vertices quand celui-ci sera plein, etc. Lors de l'appel à end_scene(), tous les vertex buffers sont dessinés via la fonction draw_array() du _RenderDevice_. J'ai remarqué que c'est plus rapide de remplir plusieurs vertex buffer que de flush à chaque fois qu'un unique vertex buffer est plein. L'utilisation de la fonction map() est aussi plus avantageux que des appels répétés à stream().
De fait on a autant de draw calls que de vertex buffers, toute la géométrie a été condensée dans quelques gros objets, c'est du batching.

Une petite optimisation a lieu dans un geometry shader : j'y duplique le triangle inférieur pour construire le triangle supérieur. C'est la raison pour laquelle je n'ai besoin de soumettre que la moitié du quad :

```glsl
layout(triangles) in;
layout(triangle_strip, max_vertices = 6) out;

in vec3 v_color[];
out vec3 f_color;

void main()
{
    for(int ii=0; ii<gl_in.length(); ii++)
    {
        gl_Position = gl_in[ii].gl_Position;
        f_color = v_color[ii];

        EmitVertex();
    }
    EndPrimitive();

    gl_Position = gl_in[2].gl_Position;
    f_color = v_color[2];
    EmitVertex();

    gl_Position = vec4(gl_in[0].gl_Position.x, gl_in[2].gl_Position.y, 0.f, 1.f);
    f_color = v_color[1];
    EmitVertex();

    gl_Position = gl_in[0].gl_Position;
    f_color = v_color[0];
    EmitVertex();
    EndPrimitive();
}
```

![Application Sandbox utilisant le batch renderer.\label{figBatchRendering}](../../Erwin_rel/screens_erwin/erwin_0a_2d_batch_rendering.png)

##2D Instance Renderer
Le deuxième renderer que j'ai codé utilise une approche différente : l'instanciation. _InstanceRenderer2D_ fonctionne sur le même genre de dynamique que le précédent, mais crée un SSBO par batch. A chaque appel à draw_quad, les données en argument (la position, l'échelle et la couleur) sont simplement empilées dans un vector de struct _InstanceData_. Quand ce vector est plein, il est déchargé dans un SSBO via la fonction map(). Le batch suivant est créé, rempli, déchargé dans un autre SSBO etc.
Lors de end_scene(), pour chaque SSBO qu'on attache au shader, un SEUL quad (contenu dans un vertex buffer) est soumis au _RenderDevice_ via draw_indexed_instanced() (qui en sous-main appèle glDrawElementsInstanced()).
Dans le shader on peut accéder aux données par-instance via l'indice gl_InstanceID.

J'ai eu une grosse emmerde dans un premier temps (affichage décalé / buggé), que j'ai bien mis 30 minutes à piger. En remarquant que le premier quad était toujours bien dessiné mais jamais les suivants, j'ai compris que **l'alignement des données dans le layout du SSBO est méga important** C'est la raison pour laquelle le membre color est un vec4 et non un vec3, pour que la taille totale d'une struct tienne sur 32 bytes (padding).

![Application Sandbox utilisant l'instanced renderer.\label{figInstancedRendering}](../../Erwin_rel/screens_erwin/erwin_0b_2d_instanced_rendering.png)

###Comparaison
L'application sandbox possède un GUI minimaliste qui permet d'ajuster dynamiquement la taille de la grille de quads, la taille maximale des batchs et l'implémentation de _Renderer2D_ utilisée ! Elle affiche des statistiques sous forme de courbes et de texte. J'ai aussi un mode qui permet de faire bouger chaque quad indépendamment (en mode onde stationnaire) côté CPU, ce qui n'a aucun effet sur le temps de rendu.

Il me faudra faire des mesures plus précises, mais en gros, le deuxième est un peu plus lent que le premier mais beaucoup plus robuste à un grand nombre de quads (le premier fait chuter les FPS parce qu'il semble CPU-bound !).
On s'attend à environ 300µs et 2 draw calls (batch size=8192) pour 10 000 quads dans les deux cas, pour 50 000 c'est 7 draw calls, le _BatchRenderer2D_ prend 500µs contre 1000µs pour le _InstanceRenderer2D_. Au-delà de 50 000 quads le _BatchRenderer2D_ est CPU-bound et fait chuter les FPS tandis que le second tient bon jusqu'à environ 140 000 quads qu'il peut rendre en environ 1.5ms avec 17 draw calls.

Côté mémoire, c'est l'instanciation qui l'emporte haut la main. Cette méthode est aussi beaucoup plus adaptée pour le passage de données par-instance au shader.


NOTE: Quand j'attaquerai la partie texture avec le _BatchRenderer2D_, je pourrai générer les UVs depuis le geometry shader, pas la peine de les stocker en attributs de vertex !


###Sources:
    [1] https://www.khronos.org/opengl/wiki/Shader_Storage_Buffer_Object
    [2] https://www.geeks3d.com/20140704/tutorial-introduction-to-opengl-4-3
        -shader-storage-buffers-objects-ssbo-demo/



#[20-09-19]
##Texture atlas & Font atlas
J'ai dev un petit outil d'atlas packing nommé FudgePacker (!) capable de contruire un atlas optimal depuis un ensemble d'images sources non-nécessairement carrées, ou bien d'une police de caractères. Fudge peut exporter les atlas en PNG avec un fichier texte pour les données de remapping : clé / cooronnées / tailles pour les texture atlases, ou bien clé / coordonnées / tailles / advance / bearing pour les font atlases. Alternativement pour les texture atlases, toutes ces donnée peuvent êtres exportées vers un format custom (CAT, pour Compressed ATlas) qui gère la compression DXT5 pour la texture et possède une table de remapping. Fudge est configurable via le fichier fudge.ini  du dossier config (parsing avec la lib IniH), il suffit d'y préciser où se trouvent les assets à packer et le type de compression :

```ini
    [paths]
    upack=source/Applications/Sandbox/assets/textures/atlas/upack
    fonts=source/Applications/Sandbox/assets/textures/atlas/upack
    [output]
    texture_compression=DXT5
    font_compression=none
```

![FudgePacker et les atlas qu'il génère.\label{figFudgePacker}](../../Erwin_rel/screens_erwin/erwin_2a_fudge_packer.png)

La lib RectPack2D est utilisée pour établir le bin packing optimal pour un ensemble de rectangles donnés. Le font rasterizing est effectué au moyen de la lib Freetype. L'export PNG se fait grâce à la lib stb_image et stb_dxt permet la compression DXT5 de blocks de pixels 4x4.

Algo pour la génération d'un atlas de textures :

    * Pour chaque sous dossier D contenant des textures à packer dans un même atlas :
        * Pour chaque fichier image I.png du dossier D :
            * Charger les données bitmaps et la taille de I dans une structure s
            * Pousser s dans un vecteur S<s>
            * Pousser un rectangle r={0,0,w,h} dans un vecteur R<r> (même indice que s dans S<s>)
                - Noter que les coordonnées x,y de r sont laissées à 0, elles 
                seront modifiées par RectPack2D directement
        * Chercher le meilleur bin packing depuis la collection R<r>
            - Après cette étape, chaque r possède des coordonnées x et y initialisées
        * Exporter les données

Algo pour la génération d'un atlas de caractères :

    * Charger une police F.ttf
        * Pour chaque caractère c de taille non nulle de la police F :
            * Charger les données bitmaps et les différents paramètres de c dans une structure s
            * Pousser s dans un vecteur S<s>
            * Pousser un rectangle r={0,0,w,h} dans un vecteur R<r> (même indice que s dans S<s>)
        * Chercher le meilleur bin packing depuis la collection R<r>
        * Exporter les données

L'écriture du blob texture pour export avec stb_image au format PNG fonctionne comme suit :
```cpp
    for(int ii=0; ii<images.size(); ++ii)
    {
        const ImageData& img = images[ii];

        for(int yy=0; yy<img.height; ++yy)
        {
            int out_y = img.y + yy;
            for(int xx=0; xx<img.width; ++xx)
            {
                int out_x = img.x + xx;
                int offset = 4 * (out_y * out_w + out_x);
                output[offset + 0] = img.data[4 * (yy * img.width + xx) + 0]; // R channel
                output[offset + 1] = img.data[4 * (yy * img.width + xx) + 1]; // G channel
                output[offset + 2] = img.data[4 * (yy * img.width + xx) + 2]; // B channel
                output[offset + 3] = img.data[4 * (yy * img.width + xx) + 3]; // A channel
            }
        }
    }
```
Chaque pixel est au format RGBA (4 bytes per pixel), et l'offset de chaque pixel (x,y) dans une image de taille (w,h) est donné par 4 * (y * w + x).

###Sources:
    [1] https://www.researchgate.net/publication/259000525_Real-Time_DXT_Compression
    [2] http://kylehalladay.com/blog/tutorial/2016/11/04/Texture-Atlassing-With-Mips.html
    [3] https://0fps.net/2013/07/09/texture-atlases-wrapping-and-mip-mapping/


##Camera & Camera controller
Contrairement à ce que je faisais dans WCore, j'applique ici une séparation rigoureuse entre la caméra (maths + data) et le contrôleur (wrapper autour de la caméra, réagit aux inputs/events). La classe _OrthographicCamera2D_ représente une caméra orthographique initialisée avec un frustum rectangulaire de type _Frustum2D_. Elle possède un ensemble de fonctions pour récupérer les matrices de vue et de projection. Sa fonction privée update_view_matrix() réalise la mise à jour des matrices vue et vue-projection lorsque la position où l'angle a changé. La fonction set_projection() permet de recalculer une projection depuis un nouveau frustum. Cette classe fait parti des classes de rendu, et doit être passée au renderer (_Renderer2D_) via begin_scene(). Le renderer se charge de lui faire crâcher ses matrices, les sauvegarde de côté, et les refile au shader lors du flush().
_OrthographicCamera2DController_ est un wrapper sur _OrthographicCamera2D_. Cette classe est utilisable côté client. Elle track un aspect ratio et un zoom level, à partir desquels elle génère un nouveau frustum pour son membre caméra à l'appel d'update(). La fonction update() fait également de l'input polling pour réagir aux événements clavier. Elle possède également deux fonctions pour réagir aux événements de redimensionnement de la fenêtre et au scroll souris (pour le zoom). Rien de plus à ajouter, c'est assez basique.

##2D Frustum culling
La classe _OrthographicCamera2D_ peut renvoyer une structure _FrustumSides_ qui contient les coefficients des droites des côtés de son frustum (en world space). Côté _Renderer2D_, à chaque soumission d'un nouveau quad, on vérifie si ce dernier (donné en world space) est dans le frustum. L'algo est similaire à celui utilisé en 3D :

    * Pour chaque côté du frustum, coefficients (a,b,c) :
        * Si tous les points du quad sont au dessus du même côté
            - On cull
        * Sinon
            - On pousse le quad

La distance signée d'un point $(x_0,y_0)$ à une droite de coefficients $(a,b,c)$ (d'équation $ax+by+c=0$) est donnée par :
    $\frac{ax_0+by_0+c}{\sqrt{a^2+b^2}}$
Il suffit dont de vérifier le signe du numérateur, qui est le produit scalaire du vecteur $(a,b,c)$ avec le point en coordonnées homogènes 2D $(x_0,y_0,1)$ :

```cpp
    // For each frustum side
    for(uint32_t ii=0; ii<4; ++ii)
    {
        // Quad is considered outside iif all its vertices are above the SAME side
        bool all_out = true;
        for(const auto& p: points)
        {
            // Check if point is above side
            if(glm::dot(fs.side[ii],p)>0)
            {
                all_out = false;
                break;
            }
        }
        if(all_out)
            return true;
    }

    return false;
```
Il m'a semblé plus économe de procéder ainsi, plutôt que de passer chaque quad en view space (les multiplications matricielles sur CPU sont à utiliser avec modération).



#[25-09-19]
##Rendu texturé
Je me rends compte que je n'ai pas documenté le rendu de textures avec _BatchRenderer2D_ et _InstancedRenderere2D_ alors que c'est fait depuis longtemps.
Il y a peu de choses à dire. J'ai modifié (je pense temporairement) la fonction Renderer2D::begin_scene() pour prendre en argument une seule texture (un atlas), et le troisième argument de draw_quad() est maintenant une paire de coordonnées UVs (bottom-left et top right) dans un vec4. De telles coordonnées sont obtenues via la fonction TextureAtlas::get_uvs() en fournissant un hash string en argument (la clé de la table de ramapping, le nom du fichier image avant qu'il ne se retrouve dans l'atlas). Seules quelques modifications mineures ont été nécessaires dans les shaders. La seule qui mérite d'être notée est celle apportée au vertex shader de _InstancedRenderere2D_ :
```glsl
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;

out vec2 v_uv;
out vec3 v_color;

struct InstanceData
{
    vec2 offset;
    vec2 scale;
    vec4 uvs;
};

layout(std430) buffer instance_data
{
    InstanceData inst[];
};

uniform mat4 u_view_projection;
out vec2 v_uv;

void main()
{
    vec3 offset = vec3(inst[gl_InstanceID].offset, 0.f);
    vec3 scale  = vec3(inst[gl_InstanceID].scale, 1.f);
    vec4 uvs    = inst[gl_InstanceID].uvs;

    gl_Position = u_view_projection*vec4(in_position*scale + offset, 1.f);
    v_uv.x = (in_uv.x < 0.5f) ? uvs.x : uvs.z;
    v_uv.y = (in_uv.y < 0.5f) ? uvs.y : uvs.w;
}
```

![Un rendu texturé (tiled rendering) depuis un atlas.\label{figAtlasRendering}](../../Erwin_rel/screens_erwin/erwin_3b_atlas_2d_rendering.png)

Comme mon SSBO ne contient que les coordonnées des coins inférieur-gauche et supérieur-droit de la sous-texture dans l'atlas et que ces données sont per-instance, il me faut regénérer les coordonnées UVs per-vertex pour pouvoir les transmettre au fragment shader. Pour celà, je me sers des coordonnées UVs en attribut de vertex 'in_uv'. Le vec4 'uvs' des données per-instance est formatté comme suit :
```
    [       x      |       y      |       z       |       w       ]
    [ U_lower_left | V_lower_left | U_upper_right | V_upper_right ]
```
Il suffit donc de comparer les composantes de 'in_uv' à 0.5 pour savoir dans quel coin je suis, et quelles composantes je dois choisir dans les per-instance UVs.
Le fragment shader est simplement :
```glsl
in vec2 v_uv;

layout(location = 0) out vec4 out_color;

uniform sampler2D us_atlas;

void main()
{
    out_color = texture(us_atlas, v_uv);
}
```

##Fudge
###Compression DXT5
FudgePacker gère donc la compression de textures DXT5. L'atlas non compressé doit d'abord être généré, comme précédemment avec les PNGs, mais en inversant la coordonnée y pour que mon loader puisse charger l'asset dans le bon ordre :
```cpp
    for(int yy=0; yy<img.height; ++yy)
    {
        out_y = inverse_y ? (height - 1) - (img.y + yy) : img.y + yy;
        // ...
    }
```
Puis pour chaque pixel de l'image, on extrait un block 4x4 dont le pixel est à la position supérieure gauche, et on applique la compression au block. Chaque block compressé tient sur 16 octets, il suffit de parcourir les pixels en row major et d'ajouter les blocks compressés les uns à la suite des autres dans un buffer, que l'on va ensuite sérialiser. Je me suis servi de [1] pour la passe de compression.

```cpp
    uint8_t* tex_blob = new uint8_t[out_w*out_h];
    memset(tex_blob, 0, out_w*out_h);

    uint8_t block[64];

    uint32_t dst_offset = 0;
    uint8_t* in_buf = uncomp;
    for(int yy=0; yy<out_h; yy+=4, in_buf+=out_w*4*4)
    {
        for(int xx=0; xx<out_w; xx+=4)
        {
            extract_block(in_buf+xx*4, out_w, block);
            stb_compress_dxt_block(&tex_blob[dst_offset], block, 1, STB_DXT_HIGHQUAL);
            dst_offset += 16;
        }
    }
```
Avec :
```cpp
inline void extract_block(const uint8_t* in_ptr, int width, uint8_t* colorBlock)
{
    for(int j=0; j<4; ++j)
    {
        memcpy(&colorBlock[j*4*4], in_ptr, 4*4 );
        in_ptr += width * 4;
    }
}
```

Je pensais au départ qu'il suffisait de découper l'image en blocks de pixels 4x4 et d'appliquer la compression sur chacun des blocks, et je m'étais fait chier le zgeg à organiser les données par block lors de la génération des données non compressées avec du calcul d'indices. Bien entendu ça n'a pas fonctionné.

La sérialisation/désérialisation des atlas de textures compressés est gérée par le format de fichiers CAT (spécifié dans core/cat_file.h). Ce format permet de stocker un blob DXT ainsi qu'une table de remapping. Il est possible qu'à l'avenir je supporte également la compression ASTC (voir [1] et [2]).

###Blob Deflate
Une passe de compression lossless optionnelle permet de réduire davantage la taille disque des fichiers CAT. L'algo Deflate de la ZLib est utilisé. Le wrapper ZLib que j'ai écrit dans core/z_wrapper.h/cpp en me servant de [3] -qui ne manque pas d'aigreur vis-à-vis des Unix geeks crasseux qui ont commis cette lib imbitable- permet de compresser/décompresser des données d'un buffer source vers un buffer destination. Dans les deux cas, la taille finale doit être connue d'avance. Dans le cas d'une compression, la taille maximale peut être estimée, elle est en réalité plus grande que la taille d'origine (compresser des données sans redondance augmente la taille), dans le cas d'une décompression c'est plus ou moins la merde (vu qu'on peut atteindre un ratio de 1/1000 dans certains cas extrêmes). Ma stratégie est de sauvegarder la taille d'origine du blob texture (DXT) dans le CAT header, ainsi j'alloue exactement la bonne taille de buffer au chargement des atlas lors de l'étape de décompression du blob.
Ce sont les fonctions read_cat() et write_cat() de cat_file.h qui gèrent la (dé)compression en sous-main, en fonction du descripteur à l'écriture et du header à la lecture.

Le moteur peut maintenant charger des assets au format PNG (et s'attend à trouver un .txt du même nom contenant les données de remapping) comme au format CAT, sans différence visible entre les deux.

###Sources:
    [1] https://developer.nvidia.com/astc-texture-compression-for-game-assets
    [2] https://github.com/ARM-software/astc-encoder/tree/master/Source
    [3] https://www.experts-exchange.com/articles/3189/In-Memory-Compression
        -and-Decompression-Using-ZLIB.html

##Logger: Single-threaded mode
Le logger est maintenant configurable pour fonctionner uniquement sur le thread principal. Appeler WLOGGER.set_single_threaded(true) passera le logger sous ce mode (veiller à le faire avant de spawn()). Le logger reste cependant thread-safe (a priori, je continue d'utiliser des mutexes), simplement, au lieu de spawner un thread machine à état, chaque appel à LoggerThread::enqueue() aura pour conséquence un dispatch immédiat, et tout appel à spawn(), flush(), sync() et kill() n'aura aucun effet. J'imagine que le côut du branchement est virtuellement nul du fait de l'exécution spéculative.
_Application_ utilise maintenant un parseur IniH pour lire un fichier de config erwin.ini, ce qui permet de changer de mode facilement (et aussi de paramétrer quels sont les événements à tracker) :
```ini
    [logger]
    backtrace_on_error=1
    single_threaded=0
    track_window_close_events=1
    track_window_resize_events=1
    track_keyboard_events=0
    track_mouse_button_events=1
    track_mouse_scroll_events=0
    track_mouse_moved_events=0
```
En retrouvant un mode immédiat, j'ai à nouveau la possibilité de faire du printf-debugging comme dans WCore, et j'en suis très heureux !

##Mandelbrot Explorer
J'ai codé il y a quelques jours une petite application qui permet l'exploration de la fractale de Mandelbrot au moyen d'un simple shader. L'idée d'origine et l'effet visuel que je tentais de reproduire sont dus à une vidée de The Art of Code (voir [1]).

![Meanwhile somewhere in Mandelbrot world...\label{figFractal}](../../Erwin_rel/screens_erwin/erwin_1c_fractal_explorer.png)


###Sources:
    [1] https://www.youtube.com/watch?v=zmWkhlocBRY


#[28-09-19]
##WRef et WScope
J'ai fini par craquer avec tous les std::shared_ptr<TrucMuche> et std::unique_ptr<MesCouilles>. Pour me faire gagner du temps à l'écriture j'ai écrit des alias pour ces types dans core/core.h, à la manière de ce qu'à fait Cherno dans Hazel (son choix de sémantique avec Ref et Scope m'a bien plu). Mais j'ai aussi codé des alias pour std::make_shared<> et std::make_unique<> :

```cpp
namespace erwin
{
    // Ref counting pointer and unique pointer aliases (for now)
    template <class T>
    using WRef = std::shared_ptr<T>;

    template <class T>
    using WScope = std::unique_ptr<T>;

    // Factory methods for ref and scope as function alias, using perfect forwarding (for now)
    template <class T, class... Args>
    auto make_ref(Args&&... args) -> decltype(std::make_shared<T>(std::forward<Args>(args)...))
    {
      return std::make_shared<T>(std::forward<Args>(args)...);
    }

    template <class T, class... Args>
    auto make_scope(Args&&... args) -> decltype(std::make_unique<T>(std::forward<Args>(args)...))
    {
      return std::make_unique<T>(std::forward<Args>(args)...);
    }
} // namespace erwin
```

##Texture2D totalement configurable
La classe _Texture2D_ possède une nouvelle factory method prenant en argument un _Texture2DDescriptor_. Un tel descripteur spécifie les dimensions, un pointeur vers d'éventuelles données, un format d'image, un filtre (min & mag), un mode d'UV wrapping et une option de lazy mipmapping. La création de la texture côté OpenGL ressemble à ce que je faisais dans WCore, mais sans le délire profond des texture units et en utilisant les fonctions plus modernes de l'API.

##Framebuffer
Je viens de terminer l'implémentation OpenGL des framebuffers. Comme d'hab, on a une interface _Framebuffer_ avec une factory method create() et une implémentation driver specific du nom de _OGLFramebuffer_. Le moteur supporte les multiple render targets, et donc il fallait trouver un moyen (plus élégant que sous WCore) d'initialiser plusieurs textures servant de color buffer au sein du framebuffer. C'est le rôle de la classe _FrameBufferLayout_, qui sur le modèle de _BufferLayout_ est un conteneur itérable de _FrameBufferLayoutElement_. Chaque élément spécifie un format d'image, un filtre et un mode d'UV wrapping.
_Framebuffer_ crée sur place les textures nécessaires, plutôt que de les attendre en argument. Ces textures sont stockées dans un vecteur et pourront être accédées par la suite. Le layout est d'abord itéré pour construire les textures associées aux color buffers une à une, et celles-ci sont attachées au framebuffer. La liste des color buffers est ensuite spécifiée, grâce à la liste d'attachments construite dans la boucle précédente. S'il est nécessaire de créer une texture pour le depth attachment ou le depth-stencil attachment, alors celle-ci est créée, attachée et poussée en fin de vecteur. Si aucune depth texture n'est demandée, alors un render buffer est créé pour servir de z-buffer, afin que le framebuffer puisse être complet. Le statut du framebuffer est alors vérifié, et la construction terminée.

##FramebufferPool
Certains framebuffers doivent tracker la taille du viewport et maintenir leur taille en fonction de celle-ci. Par exemple, un effect framebuffer utilisé pour le post-processing suivra à l'exact la taille du viewport, tandis qu'un autre framebuffer utilisé pour une blur pass pourra suivre cette taille avec un ratio 1:2. D'autres framebuffers, comme dans le cas du shadow mapping, auront une taille fixe.
La nécessité de redimensionner certains framebuffers lorsque le viewport change (suite à un redimensionnement de la fenêtre par exemple), me force à centraliser tous les framebuffers dans un conteneur statique qui répond aux événements de redimensionnement. C'est précisément le rôle de la _FramebufferPool_ qui est créée en début d'application et détruite en sortie de game loop. Une instance unique de _FramebufferPool_ appartient à la classe statique _Gfx_ qui host également le render device. Cette classe associe les framebuffers qu'elle héberge aux contraintes de taille auxquelles ils obéissent. Ces contraintes sont spécifiées par des classes de contrainte de type _FbConstraint_.

_FbConstraint_ se décline pour le moment en deux classes spécialisées : _FbFixedConstraint_ pour les framebuffers de taille fixe, et _FbRatioConstraint_ pour les framebuffers dont la taille est en rapport algébrique avec la taille du viewport. Un rapport de 1:1 est toujours spécifié par une contrainte de type _FbRatioConstraint_, avec des coefficients multiplicateurs unitaires (défaut). Les méthodes virtuelles FbConstraint::get_width(1) et FbConstraint::get_height(1) retournent les dimensions souhaitées en fonction des dimensions du viewport.

Chaque framebuffer doit être créé via la méthode FramebufferPool::create_framebuffer(5). Cette méthode prend en argument une intern string pour faire référence au framebuffer par la suite, une contrainte, un _FrameBufferLayout_, ainsi que deux booléens qui précisent la nécessité de créer une depth/depth-stencil texture :

```cpp
FrameBufferLayout layout_0 =
{
    {"target1"_h, ImageFormat::RGBA8, MIN_LINEAR | MAG_NEAREST, TextureWrap::REPEAT}
};
Gfx::framebuffer_pool->create_framebuffer("fb_ratio_05"_h, 
    make_scope<FbRatioConstraint>(0.5f,0.5f), layout_0, false);

FrameBufferLayout layout_1 =
{
    {"albedo"_h, ImageFormat::RGBA16F, MIN_LINEAR | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE},
    {"normal"_h, ImageFormat::RGB16_SNORM, MIN_LINEAR | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE}
};
Gfx::framebuffer_pool->create_framebuffer("fb_ratio_025_05"_h, 
    make_scope<FbRatioConstraint>(0.25f,0.5f), layout_1, true);
```
Les framebuffers peuvent être bound directement depuis la méthode FramebufferPool::bind(hash_t).

_FramebufferPool_ réagit aux événements _FramebufferResizeEvent_, publiés par le callback GLFW *FramebufferSizeCallbackFun* (dans _GLFWWindow_). A la réception d'un tel événement, chaque framebuffer non fixe est détruit et recréé avec les mêmes paramètres qu'avant, sauf la taille qui est mise à jour.

#[28-09-19]
##Framebuffer opérationnel
J'ai testé la fonctionnalité de _Framebuffer_ dans _Renderer2D_ en implémentant une passe (mockup) de post-processing. Un framebuffer du nom de "fb_2d_raw" est créé dans le constructeur via :
```cpp
if(!Gfx::framebuffer_pool->exists("fb_2d_raw"_h))
{
    FrameBufferLayout layout =
    {
        {"albedo"_h, ImageFormat::RGBA8, MIN_LINEAR | MAG_NEAREST, 
            TextureWrap::CLAMP_TO_EDGE}
    };
    Gfx::framebuffer_pool->create_framebuffer("fb_2d_raw"_h, 
        make_scope<FbRatioConstraint>(), layout, false);
}
```
Un shader de post-processing (simple pass-through pour l'instant) est aussi chargé par la _ShaderBank_, et un quad de la taille de l'écran est sauvegardé dans un vertex array :
```cpp
shader_bank.load("shaders/post_proc.glsl");

// Create vertex array with a quad
BufferLayout vertex_tex_layout =
{
    {"a_position"_h, ShaderDataType::Vec3},
    {"a_uv"_h,       ShaderDataType::Vec2},
};
float sq_vdata[20] = 
{
    -1.0f, -1.0f, 0.0f,   0.0f, 0.0f,
     1.0f, -1.0f, 0.0f,   1.0f, 0.0f,
     1.0f,  1.0f, 0.0f,   1.0f, 1.0f,
    -1.0f,  1.0f, 0.0f,   0.0f, 1.0f
};
uint32_t sq_idata[6] =
{
    0, 1, 2,   2, 3, 0
};
auto quad_vb = VertexBuffer::create(sq_vdata, 20, vertex_tex_layout);
auto quad_ib = IndexBuffer::create(sq_idata, 6, DrawPrimitive::Triangles);
screen_va_ = VertexArray::create();
screen_va_->set_index_buffer(quad_ib);
screen_va_->set_vertex_buffer(quad_vb);
```

Voici le code du post-processing fragment shader :
```glsl
in vec2 v_uv;
layout(location = 0) out vec4 out_color;

uniform sampler2D us_input;

void main()
{
    vec4 in_color = texture(us_input, v_uv);
    // in_color.r = (v_uv.x<0.5) ? in_color.r : 0.f;
    out_color = in_color;
}
```

Dans la fonction end_scene(), la fonction flush() est appelée après un framebuffer switch affin de dessiner dans une texture (offscreen). Le framebuffer par défaut est ensuite bound, ainsi que le shader de post-proc, la texture du framebuffer "fb_2d_raw" est liée et passée en uniform sampler au shader, puis le quad écran est dessiné :
```cpp
// Render on offscreen framebuffer
Gfx::framebuffer_pool->bind("fb_2d_raw"_h);
Gfx::device->clear(CLEAR_COLOR_FLAG);
flush();
// Render generated texture on screen after post-processing
Gfx::framebuffer_pool->bind(0);
const Shader& post_proc_shader = Renderer2D::shader_bank.get("post_proc"_h);
post_proc_shader.bind();
auto&& albedo_tex = Gfx::framebuffer_pool->get_named_texture("fb_2d_raw"_h, "albedo"_h);
post_proc_shader.attach_texture("us_input"_h, albedo_tex);
Gfx::device->draw_indexed(screen_va_);
post_proc_shader.unbind();
```

![Test de "post-processing", le canal rouge est annulé sur la droite de l'écran.\label{figTestPP}](../../Erwin_rel/screens_erwin/erwin_4a_test_post_proc.png)


##De l'intérêt du teddy bear debugging
Lors de mon implémentation de la passe de post-processing dans _Renderer2D_ j'ai eu droit à un violent segfault lors du premier draw call après un renderer swap vers _InstancedRenderer2D_. Le seul bout de code que j'avais ajouté était un vertex array en membre privé de _Renderer2D_, initialisé dans le constructeur. Le vertex array utilisé par _InstancedRenderer2D_ contient un simple quad utilisé pour l'instancing, et c'est celui-là qui foutait la merde lors de l'appel à draw_indexed_instanced(). Les deux vertex array sont indépendant du point de vue de mon code.
GDB ne m'a pas beaucoup aidé : j'avais identifié le dernier call avant le segfault après un backtrace, point barre. En discutant avec Jess j'ai établi que ce type de side-effect ressemblait beaucoup à du state leaking OpenGL. Je suis donc allé voir dans render/ogl_buffer.cpp, plus particulièrement dans OGLVertexArray::add_vertex_buffer() et OGLVertexArray::set_index_buffer(), et à tout hasard, rajouté un glBindVertexArray(0) après les divers calls OpenGL pour unbind le vertex array immédiatement après y avoir touché. Surprise : tout fonctionne de nouveau, le renderer swap ne fait plus rien planter.
Vraisemblablement, le state leaking avait entraîné que le vertex buffer ou l'index buffer d'un des deux vertex array se retrouvait lié dans l'état du second. Après destruction de ces VBO/IBO lors du swap, je devais me retrouver avec un vertex array pointant sur des objets détruits, d'où le segfault... Merci Jess de m'écouter jargonner !


#[30-09-19]
##Atlas bleeding
Mon remapping était assez naïf: au chargement d'un atlas je calculais simplement les coordonnées UVs des coins inférieur gauche et supérieur droit pour chaque sous-texture. Ceci entraînait que sur les bords d'une sous-texture, le sampler interpolait avec la sous-texture d'à côté, provoquant un léger bleeding. En réalité, il faut adresser les centres des texels plutôt que les bords (voir [1] et [2]). Pour ceci, une simple translation positive d'un demi texel pour le coin inférieur gauche et négative d'un demi texel pour le coin supérieur droit suffit à arranger le problème :
```cpp
// Calculate UVs for bottom left and top right corners
// Also apply half-pixel correction to address the texel centers and avoid bleeding
glm::vec4 uvs((x+0.5f)/width, (y+0.5f)/height, (x-0.5f+w)/width, (y-0.5f+h)/height);
// Save uvs in remapping table
remapping_.insert(std::make_pair(H_(key.c_str()), uvs));
```
Cette technique s'appèle *half-pixel correction*.

![Pas bien.\label{figAtlWrongUV}](../../Erwin_rel/Figures/atlas_wrong_UVs.png){width=5cm height=5cm}
![Bien.\label{figAtlOkUV}](../../Erwin_rel/Figures/atlas_ok_UVs.png){width=5cm height=5cm}


###Sources:
    [1] https://gamedev.stackexchange.com/questions/46963/how-to-avoid-texture
        -bleeding-in-a-texture-atlas
    [2] https://docs.microsoft.com/en-us/windows/win32/direct3d9/directly
        -mapping-texels-to-pixels?redirectedfrom=MSDN


#[02-10-19]
##XML files
J'ai retrouvé toutes les fonctionnalités de parsing des fichiers XML que j'avais développées sous WCore, mais en un peu mieux codées. La structure _XMLFile_ possède un constructeur qui enregistre un chemin d'accès. L'appel à la fonction read() va lire le fichier ciblé, le parser, et initialiser les quelques membres (DOM, buffer, racine). Cette structure possède aussi quelques fonctions pour modifier le DOM et une fonction d'écriture.

```cpp
    xml::XMLFile cfg(filepath);
    if(!cfg.read())
        // ...
    auto* opt_node = cfg.root->first_node("Options");
    if(xml::parse_node(opt_node, "BlobCompression", blob_compression_str))
        // ...
    for(auto* tmap_node=tmaps_node->first_node("TextureMap");
        tmap_node;
        tmap_node=tmap_node->next_sibling("TextureMap"))
    {
        // ...
        xml::parse_attribute(tmap_node, "name", spec.name);
        // ...
```

##TOM files
Hier j'ai codé un nouveau format de fichiers capable de contenir les différentes textures maps d'un même material. Le fichier contient un header qui spécifie les dimensions (communes) des texture maps, leur nombre (N), un paramètre de wrap, le type de compression lossless pour le blob de données, la taille compressée et non compressée du blob.
Après ce header, on trouve N *Block descriptors*, chacun est spécifique à une texture map et contient un paramètre de filtrage, un nombre de color channels, une option SRGB, un type de compression de texture, une taille et un hash de nom de texture map.
Le blob de données est constitué de toutes les texture maps concaténées, compressées pour celles qui doivent l'être, en représentation row major. Le blob entier est compressible avec un algo DEFLATE.

Sur le modèle des CAT files, un TOM file peut être lu ou écrit grâce à un _TOMDescriptor_. Un tel descripteur spécifie un chemin d'accès et les divers paramètres globaux, mais en utilisant les types énumérés du moteur, plus un vecteur de _TextureMapDescriptor_, un par texture map, dans lesquels sont spécifiés les paramètres par-texture, dont un pointeur de données.

##Fudge: Texture packer
Fudge peut maintenant générer des TOM files automatiquement depuis des dossiers contenant les assets sous format image. Chaque asset voit ses texture maps rangées dans un sous dossier, chaque nom de fichier est le nom d'une map (albedo, normal, ...). L'asset produit porte le nom du sous-dossier. Un fichier de configuration au format XML permet de fixer quelques options, décrire les différentes texture maps, et également de spécifier des *groupes*.
Un groupe rassemble plusieurs texture maps dans la même texture, de sorte à minimiser le nombre de textures chargées par le moteur et transmises aux shaders. Un asset qui possède toutes les texture maps d'un même groupe est qualifié pour utiliser celui-ci, et le groupe est généré automatiquement avant l'écriture au format TOM. Un groupe est un peu à l'image de ce que j'appelais les *texture blocks* sous WCore, mais c'est l'utilisateur qui les définit, ils ne sont pas fixés par l'engine. Reste à savoir de quelle manière je vais abstraire ces structures côté moteur pour m'assurer que chaque shader trouvera toutes ses ressources automatiquement, même si celles-ci sont groupées. Le grouping peut être désactivé.

Voici un exemple de fichier de config :
```xml
<?xml version="1.0" encoding="utf-8"?>
<Config>
    <Options>
        <BlobCompression>DEFLATE</BlobCompression>
        <AllowGrouping>true</AllowGrouping>
    </Options>

    <TextureMaps>
        <TextureMap name="albedo" channels="4" compression="DXT5" 
                    filter_min="NEAREST" filter_mag="NEAREST" srgb="true"/>
        <TextureMap name="normal" channels="3" compression="None" 
                    filter_min="NEAREST" filter_mag="NEAREST" srgb="false"/>
        <TextureMap name="depth" channels="1" compression="None" 
                    filter_min="NEAREST" filter_mag="NEAREST"/>
        <TextureMap name="metal" channels="1" compression="None" 
                    filter_min="NEAREST" filter_mag="NEAREST"/>
        <TextureMap name="roughness" channels="1" compression="None" 
                    filter_min="NEAREST" filter_mag="NEAREST"/>
        <TextureMap name="ao" channels="1" compression="None" 
                    filter_min="NEAREST" filter_mag="NEAREST"/>
        
        <Group name="normal_depth" compression="DXT3" filter_min="NEAREST" 
               filter_mag="NEAREST" srgb="false">
            <TextureMap name="normal"/>
            <TextureMap name="depth"/>
        </Group>
        <Group name="mra" compression="DXT1" filter_min="NEAREST" 
               filter_mag="NEAREST" srgb="false">
            <TextureMap name="metal"/>
            <TextureMap name="roughness"/>
            <TextureMap name="ao"/>
        </Group>
    </TextureMaps>
</Config>
```
On repère deux groupes répondant aux noms de "normal_depth" et "mra". Les paramètres des groupes surchargent ceux des simples texture maps. Ainsi, un asset qui ne qualifie pas pour le grouping de "normal" et "depth" parce qu'il ne possède pas de depth map, aura sa normal map non compressée dans une texture map du nom de "normal", mais si on venait lui ajouter une depth map, alors les deux maps seraient groupées sous la texture map "normal_depth" (normal map accessible via les canaux R,G et B, depth accessible via le canal Alpha), et cette texture map serait compressée au format DXT3 (hypothétiquement, car je ne gère pas encore ce type de compression).

Exemple d'output de Fudge lors du texture packing :
```
    [0.135912][fud]  ⁕  Iterating unpacked texture maps directories.
    [0.135945][fud]  ⁕  Processing directory: "beachSand"
    [0.135971][fud]     Reading: "albedo.png"
    [0.138115][fud]      ↳ SRGB format.
    [0.138124][fud]      ↳ DXT5 compression.
    [0.138146][fud]     Reading: "ao.png"
    [0.139028][fud]     Reading: "normal.png"
    [0.141071][fud]     Reading: "metal.png"
    [0.141330][fud]     Reading: "depth.png"
    [0.142508][fud]     Reading: "roughness.png"
    [0.144397][fud]     Qualifies for grouping under layout: normal_depth
    [0.144699][fud]     Qualifies for grouping under layout: mra
    [0.147243][fud]     Exporting: beachSand.tom
    [0.159019][fud]  ⁕  Processing directory: "rockTiling"
    [0.159068][fud]     Reading: "albedo.png"
    [0.195157][fud]      ↳ SRGB format.
    [0.195183][fud]      ↳ DXT5 compression.
    [0.195198][fud]     Reading: "ao.png"
    [0.222227][fud]     Reading: "normal.png"
    [0.281457][fud]     Reading: "depth.png"
    [0.301390][fud]     Reading: "roughness.png"
    [0.320072][fud]     Qualifies for grouping under layout: normal_depth
    [0.362300][fud]     Exporting: rockTiling.tom
```

##Fudge: Asset registry
Afin d'éviter de reconstruire les assets déjà à jour, j'ai codé un petit registre des assets qui associe à une directory_entry un hash qui change à chaque fois qu'un des fichiers image d'entrée est modifié (voir asset_registry.h/cpp). L'élément clé pour rendre celà possible est de pouvoir récupérer la dernière date de modification d'un fichier via std::filesystem :
```cpp
auto ftime = fs::last_write_time(path_to_file);
std::time_t cftime = decltype(ftime)::clock::to_time_t(ftime);
```

Une directory_entry peut être un simple fichier, comme c'est le cas avec les fichiers fonts (.ttf). Dans ce cas, on calcule simplement un hash du timestamp de dernière modification de ce fichier, et on l'associe au hash de la directory_entry dans une unordered_map :
```cpp
static std::unordered_map<uint64_t, uint64_t> s_registry; // <name hash, timestamp hash>
// ...
const fs::directory_entry& entry; // = ...
uint64_t ts_hash = FAR_MAGIC;
if(entry.is_regular_file())
{
    // Compute timestamp hash
    auto ftime = fs::last_write_time(entry.path());
    std::time_t cftime = decltype(ftime)::clock::to_time_t(ftime);
    hash_combine(ts_hash, cftime);
}
// ...
s_registry.insert(std::make_pair(hname, ts_hash));
```

Dans le cas des texture atlas en revanche, la directory_entry envoyée à l'atlas packer est un dossier. Pour calculer le hash il faut itérer tout le dossier, et pour chaque fichier calculer un timestamp, et combiner un hash au fur et à mesure :
```cpp
uint64_t ts_hash = FAR_MAGIC;
if(entry.is_directory())
{
    // Iterate over all files in this directory and combine timestamp hashes
    for(auto& sub_entry: fs::directory_iterator(entry.path()))
    {
        auto sub_path = sub_entry.path();
        auto ftime = fs::last_write_time(sub_path);
        std::time_t cftime = decltype(ftime)::clock::to_time_t(ftime);
        hash_combine(ts_hash, cftime);
    }
}
// ...
```
Avec la classique fonction de combinaison des hash :
```cpp
template <class T>
inline void hash_combine(uint64_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}
```

Ces deux comportements sont implémentés derrière une seule fonction du namespace "far" :
```cpp
bool need_create(const fs::directory_entry& entry);
```

Le registre peut être chargé depuis / écrit dans un fichier .far (Fudge Asset Registry), et donc Fudge est capable de tracker les fichiers d'assets qu'il génère. Ainsi, on peut vérifier facilement si un fichier d'asset doit être construit ou non :
```cpp
for(auto& entry: fs::directory_iterator(s_tmap_upack_path))
    if(entry.is_directory() && (fudge::far::need_create(entry) || s_force_rebuild))
        fudge::texmap::make_tom(entry.path(), s_tmap_upack_path.parent_path());
```
Noter la présence du booléen statique *s_force_rebuild* qui est initialisé à *true* quand l'option "-f" (pour "force") est passée au programme. Ceci est utile, car le système ne track pas les noms d'assets écrits, seulement les directory_entry en argument des factory methods. De fait, si l'on supprime un fichier d'asset, celui-ci ne sera pas recréé en relançant fudge car il existe encore une entrée valide dans le registre. Ceci sera probablement corrigé par la suite.


#[03-10-19] Maintenance
##Glad Submodule
J'ai écrit un fichier extensions.txt contenant toutes les extensions que GLAD doit prendre en charge, à la racine du dossier glad. J'ai aussi créé un CMakeLists.txt custom qui remplace l'ancien, afin de compiler la lib en -fPIC et d'écrire le .a dans le dossier erwin/lib.

> cd source/vendor
> git submodule add https://github.com/Dav1dde/glad.git glad
> cd glad
> python -m glad --generator=c --spec=gl --profile=core --api="gl=4.6" --extensions=./extensions.txt --out-path=.
> mkdir build; cd build
> cmake ..
> make

##Linux build script
Le script scripts/build_linux.sh installe toutes les dépendances linux, initialise les submodules et les compile, puis génère toutes les targets de mon projet. En théorie, on doit pouvoir se contenter de cloner mon git et d'exécuter ce script pour tout générer :
> git clone https://github.com/ndoxx/erwin.git
> cd scripts
> sh ./build_linux.sh

NOTES:
    * Il *faut* se trouver dans le dossier scripts, sans quoi rien ne peut fonctionner (pour l'instant, je suppose). 
    * Les ressources que j'utilise pour mes tests avec la sandbox (textures...) ne sont pas sur le github.


#[06-10-19]
##SPIR-V
SPIR-V est un langage de shading intermédiaire proche du langage machine. Comme il est plus simple et moins sujet à l'interprétation que les langages de plus haut niveau tels que GLSL ou HLSL, on peut supposer que ses implémentations constructeur varient peu et sont suffisamment optimisées. L'idée est (comme avec Clang/LLVM) de couper le compilateur en deux : un front-end (GLSL/HLSL) et un back-end (SPIR-V). Un avantage immédiat est que l'on peut avoir en entrée des shaders écrits en HLSL, pour autant la sortie SPIR-V sera exécutable dans un environnement OpenGL ou Vulkan.

Fudge se charge également de la compilation des shaders dans des modules SPIR-V (non encore testés in-game). L'application a (jusqu'à nouvel ordre) recours à des system calls pour exécuter des lignes de commande impliquant différents outils SPIR-V externes. La fonction check_toolchain() du namespace fudge::spv permet de vérifier la présence de ces outils. Si un seul vient à manquer, la compilation des shaders n'aura pas lieu. Il est laissé à la discrétion de l'utilisateur de l'engine d'installer ces outils lui-même et de les rendre disponibles dans le PATH.

*glslangValidator* est utilisé pour la compilation depuis le GLSL vers SPIR-V :
> glslangValidator -G -e main -o vert.spv input.vert

*spirv-link* est utilisé pour combiner plusieurs shaders SPIR-V en un seul module binaire avec plusieurs points d'entrée :
> spirv-link vert.spv geom.spv frag.spv -o module_name.spv

*spirv-opt* sert à optimiser des modules SPIR-V :
> spirv-opt --eliminate-dead-code-aggressive [...] input.spv -o output.spv

###Travail préliminaire
Pour qu'un shader GLSL puisse compiler en SPIR-V, plusieurs contraintes doivent être respectées :
    
    * Les attributs et varying doivent être localisés
    * Les types opaques (comme sampler2D) doivent être associés à un binding
    * Tous les uniforms doivent être présents dans un layout (genre UBO) avec un binding déterminé

Exemple avec mon fragment shader de post-processing :
```glsl
#version 460 core

layout(location = 2) in vec2 v_uv;
layout(location = 0) out vec4 out_color;
layout(binding = 0) uniform sampler2D us_input;

layout(std140, binding = 0) uniform post_proc_layout
{
    vec4 u_vib_balance;     // Vibrance
    vec4 u_cor_gamma;       // Color correction
    float u_ca_shift;       // Chromatic aberration
    // ...
};
```
Basiquement, on spécifie depuis le shader absolument toutes les données d'indexation des ressources. Côté client, il faut donc récupérer ces données plutôt que de les modifier. Par exemple, jusque-là je devais préciser un binding point dans mes calls à Shader::attach_shader_storage() et Shader::attach_uniform_buffer(), et je me servais de ce paramètre pour modifier dynamiquement le binding point de la ressource en question :
```cpp
void OGLShader::attach_uniform_buffer(const UniformBuffer& buffer, 
                                      uin32_t binding_point) const
{
    hash_t hname = H_(buffer.get_name().c_str());
    glBindBufferBase(GL_UNIFORM_BUFFER, binding_point, 
        static_cast<const OGLUniformBuffer&>(buffer).get_handle());
    GLuint block_index = glGetProgramResourceIndex(rd_handle_, 
        GL_UNIFORM_BLOCK, buffer.get_name().c_str());
    glUniformBlockBinding(rd_handle_, block_index, binding_point);
}
```
C'était incontestablement une très mauvaise façon de faire, et j'ai dû y remédier. Pour celà il me fallait découvrir les bindings lors de la phase d'introspection qui a lieu après compilation de chaque shader, via la fonction OGLShader::introspect(). Cette fonction d'introspection a été réécrite de manière plus modèrne pour tirer partie des dernières fonctions de l'API : glGetProgramInterfaceiv(), glGetProgramResourceiv() et glGetProgramResourceName() ([5] m'a pas mal aidé). Par exemple, pour associer le nom d'un uniform buffer avec son binding point tel que définit dans la source du shader, on fera :
```cpp
// OGLShader member
std::unordered_map<hash_t, uint32_t> OGLShader::block_bindings_;

// ...

std::vector<GLenum> properties {GL_NAME_LENGTH, GL_BUFFER_BINDING};
// Get active UBO count
GLint num_active;
GLenum iface = GL_UNIFORM_BLOCK;
glGetProgramInterfaceiv(rd_handle_, iface, GL_ACTIVE_RESOURCES, &num_active);
// Get properties for each active UBO
std::vector<GLint> prop_values(properties.size());
std::vector<GLchar> name_data(256);
for(int jj=0; jj<num_active; ++jj)
{
    glGetProgramResourceiv(rd_handle_, iface, jj, properties.size(),
                           &properties[0], prop_values.size(), nullptr, &prop_values[0]);
    // Get UBO name
    name_data.resize(prop_values[0]); //The length of the name.
    glGetProgramResourceName(rd_handle_, iface, jj, name_data.size(), nullptr, &name_data[0]);
    std::string resource_name((char*)&name_data[0], name_data.size() - 1);
    block_bindings_.insert(std::make_pair(H_(resource_name.c_str()), prop_values[1]));
}
```
Mon implémentation de introspect() est une tentative de récupération automatique de toutes ces propriétés non-exclusivement pour les UBOs, mais aussi pour d'autres ressources d'intérêt comme les attributs, les uniformes, et les shader storage blocks. A l'issue de l'introspection, on possède une collection de tous les binding points associés aux noms des ressources. Pour attacher un uniform buffer, ou un shader storage buffer, c'est maintenant beaucoup plus simple et sécurisé :
```cpp
void OGLShader::attach_uniform_buffer(const UniformBuffer& buffer) const
{
    hash_t hname = H_(buffer.get_name().c_str());
    GLint binding_point = block_bindings_.at(hname);
    glBindBufferBase(GL_UNIFORM_BUFFER, binding_point, 
        static_cast<const OGLUniformBuffer&>(buffer).get_handle());
}
```

###Stratégie
Comme mes sources GLSL regroupent plusieurs shaders dans le même fichier et que glslangValidator attend en entrée un fichier par shader, il me faut créer autant de fichiers GLSL temporaires qu'il y a de shaders dans un programme à traîter. Un tel fichier temporaire contient uniquement du code GLSL légal, en particulier les includes doivent être évalués à ce stade. Je sais qu'il existe un moyen de faire en sorte que glslangValidator se charge des includes (avec l'extension Google include) via :
```glsl
    #extension GL_GOOGLE_include_directive : enable
```
(voir [1]) mais j'ai préféré parser les includes moi-même comme je le fais dans _OGLShader_, pour l'instant.

Le validateur compilera ensuite chacun de ces fichiers GLSL vers des fichiers .spv (temporaires eux aussi). Puis chacun de ces fichiers .spv est linké en un seul module grâce à spirv-linker. Une passe d'optimisation a ensuite lieu sur le module via spirv-opt. Le module obtenu est le produit fini de la passe de compilation de shaders par Fudge.

###Entry points
Un module SPIR-V linké contient nécessairement plusieurs points d'entrée (un par shader stage). Comme dans la source tous ces points d'entrée se nomment *main*, je me suis demandé un moment comment le linker procédait pour les différencier. Par inspection d'un module linké grâce au désassembleur *spirv-dis* on peut lire ceci pour un module contenant les trois stages vertex, geometry et fragment :
```spv
   OpCapability Shader
   OpCapability Geometry
%1 = OpExtInstImport "GLSL.std.450"
   OpMemoryModel Logical GLSL450
   OpEntryPoint Vertex %main "main" %_ %in_position %v_uv %in_uv %gl_VertexID %gl_InstanceID
   OpEntryPoint Geometry %main_0 "main" %__0 %gl_in %f_uv %v_uv_0
   OpEntryPoint Fragment %main_1 "main" %out_color %f_uv_0
   OpExecutionMode %main_0 Triangles
   OpExecutionMode %main_0 Invocations 1
   OpExecutionMode %main_0 OutputTriangleStrip
   OpExecutionMode %main_0 OutputVertices 6
   OpExecutionMode %main_1 OriginLowerLeft
```
Chaque point d'entrée semble s'appeler "main" à l'export, mais je suppose que le code de spécialisation doit pouvoir s'en sortir puisqu'on lui spécifie aussi le type de stage pour chaque point d'entrée.
Il me faut donc une fonction de parsing des opcodes correspondants afin de déterminer quels sont les shader stages présents dans le module, et quels sont les noms de leurs points d'entrée. J'aurai en effet besoin de ces informations pour pouvoir automatiser correctement la spécialisation SPIR-V de mes shaders.

Je viens de coder une telle fonction : spv::parse_stages() qui prend en argument un chemin d'accès vers un fichier .spv et retourne un vecteur de _ShaderStageDescriptor_, structures à deux membres : un type énuméré _ExecutionModel_ pour le type de shader stage, et une string pour le nom de l'entry point. Je me suis aidé de [2], mais aussi des specs de SPIR-V [3] et du header [4]. La fonction est d'ailleurs utilisée par Fudge une fois chaque module compilé, affin d'afficher chaque point d'entrée pour chaque stage. J'ai maintenant la voie libre pour commencer l'intégration côté moteur.

###Intégration
J'ai réorganisé le code de _Shader_ et _OGLShader_ pour présenter un constructeur par défaut et des méthodes d'initialisation (depuis un fichier source GLSL, depuis une string GLSL et depuis un module SPIR-V). C'est la factory method create qui appèle la bonne méthode d'initialisation en fonction de l'extension du fichier, pour différencier une source GLSL d'une source SPIR-V.

Créer un shader SPIR-V en OpenGL est assez simple. Tout d'abord, il faut charger le fichier binaire dans un buffer. Puis pour chaque shader stage, on génère un shader comme d'habitude avec glCreateShader(), on appèle glShaderBinary() pour uploader le binaire dans OpenGL, puis glSpecializeShader() pour spécialiser le shader en précisant son point d'entrée. La spécialisation équivaut à une compilation :
```cpp
std::vector<uint8_t> spirv;
// Load spir-v binary inside vector
filesystem::get_file_as_vector(filepath, spirv);

std::string entry_point("main");
GLenum type = GL_VERTEX_SHADER;
GLuint shader_id = glCreateShader(type);
glShaderBinary(1, &shader_id, GL_SHADER_BINARY_FORMAT_SPIR_V, spirv.data(), spirv.size());
glSpecializeShader(shader_id, (const GLchar*)entry_point.c_str(), 0, nullptr, nullptr);
```
Le rapport d'erreur de compilation s'obtient comme pour n'importe quel shader, et la procédure de linking du programme ne change pas non plus.

Je peux maintenant charger des shaders SPIR-V. J'ai cependant des problèmes avec color_dup_shader.spv et mandelbrot.spv qui produisent un output délirant, sans doute à cause d'un layout pourri qui fonctionne sous OpenGL et plus sous SPIR-V (j'ai pu tester que la view-projection matrix était en cause dans mandelbrot.spv, en virant la multiplication par celle-ci, tout fonctionne).
J'ai un launcher script pour la sandbox qui me permet d'exécuter Fudge juste avant. Grâce à l'asset registry, cette exécution est très rapide.

###Build
Pour le validateur/compilateur :
> git clone https://github.com/KhronosGroup/glslang.git glslang
> cd glslang
> ./update_glslang_sources.py
> mkdir build
> cd build
> cmake ..
> make -j4
> sudo make install

Pour les différents outils SPIR-V :
> git clone https://github.com/KhronosGroup/SPIRV-Tools.git   spirv-tools
> git clone https://github.com/KhronosGroup/SPIRV-Headers.git spirv-tools/external/spirv-headers
> cd spirv-tools
> mkdir build
> cd build
> cmake ..
> make -j4
> sudo make install

###Sources:
    [1] https://github.com/KhronosGroup/glslang/issues/1691
    [2] https://www.leadwerks.com/community/blogs/entry/2403-single-file-spir-v
        -shaders-for-vulkan/
    [3] https://www.khronos.org/registry/spir-v/specs/unified1/SPIRV.pdf
    [4] https://github.com/KhronosGroup/SPIRV-Headers/blob/master/include
        /spirv/unified1/spirv.hpp11
    [5] https://www.khronos.org/opengl/wiki/Program_Introspection#Interface_query

#[11-10-19]
##Config
core/config.h/cpp définissent un ensemble de fonctions pour :

    * importer des paires clé/valeur depuis un fichier de config XML
    * récupérer une valeur d'un certain type contre une clé

Ce système permet aussi d'automatiser la configuration du logger pour la déclaration des channels et des sinks. Exemple de configuration :
```xml
<?xml version="1.0" encoding="utf-8"?>
<Config>
    <logger>
        <Channel name="application" verbosity="3"/>
        <Channel name="render"      verbosity="3"/>
        <!-- ... -->
        <Sink name="ConsoleSink"    type="ConsoleSink" channels="all"/>
        <Sink name="MainFileSink"   type="LogFileSink" channels="all"   destination="erwin.log"/>
        <Sink name="EventFileSink"  type="LogFileSink" channels="event" destination="events.log"/>
        <!-- ... -->
        <bool name="backtrace_on_error" value="false"/>
        <bool name="single_threaded" value="true"/>
        <bool name="track_window_close_events" value="true"/>
        <!-- ... -->
    </logger>
    <display>
        <uint name="width"  value="1920"/>
        <uint name="height" value="1080"/>
        <!-- ... -->
    </display>
</Config>
```
Dans le cas des sinks, l'attribut *channels* peut être omis ou de manière équivalente fixé à "all" pour attacher le sink en question à tous les canaux. Pour attacher un sink à un seul canal ou bien à une liste de canaux, *channels* sera affecté d'un nom de canal, ou bien d'une liste séparée par des virgules.

##Maintenance
###Fudge batches
Fudge est configuré via un fichier XML :
```xml
<?xml version="1.0" encoding="utf-8"?>
<Config>
    <atlas>
        <texture>
            <batch input="source/Applications/Sandbox/assets/textures/atlas/upack"
                   output="source/Applications/Sandbox/assets/textures/atlas">
                <texture_compression>DXT5</texture_compression>
                <blob_compression>deflate</blob_compression>
            </batch>
        </texture>
        <!-- ... -->
    </atlas>
    <!-- ... -->
    <shader>
        <batch input="source/Erwin/assets/shaders"
               output="source/Erwin/assets/shaders"/>
        <batch input="source/Applications/FractalExplorer/assets/shaders"
               output="source/Applications/FractalExplorer/assets/shaders"/>
    </shader>
</Config>
```
Chaque type d'asset que Fudge peut traiter peut définir plusieurs batches. Chaque batch précise une paire de chemins d'accès (entrée / sortie). Certains batches peuvent définir des propriétés locales comme les types de compression à utiliser pour les texture atlases. Fudge itère chaque batch correctement défini, pour chaque catégorie d'asset.

#[26-10-19]
Merde, ce retard de dingue dans ma prise de notes...

##Memory System
Je me suis beaucoup inspiré des articles [1] à [8] pour réaliser un système de gestion de la mémoire assez puissant. L'idée générale consiste à réserver un gros bloc mémoire, et à utiliser une ou plusieurs "arènes" sur ce bloc pour allouer dynamiquement les objets que l'on veut avec du placement new.

##Area & Arena
Un espace mémoire préalloué est décrit par une *surface*. Pour l'instant, je ne gère que des allocations sur le tas grâce à la surface _HeapArea_, mais rien n'empêche de créer par exemple une surface _StackArea_ qui réserve un espace sur la pile (via alloca() par exemple). Une surface mémoire implémente un allocateur linéaire très basique, et dispose d'une fonction require_block() qui réserve un bloc mémoire et retourne une paire de pointeurs (begin / end) qui encadre ce bloc. Le deuxième pointeur ("end") pointe vers l'adresse immédiatement __après__ le bloc. Par ailleurs, le pointeur vers le début du bloc est toujours aligné sur 64 octets (*page-aligned*), de sorte à éviter le *false sharing* lorsque deux threads accèdent à des blocs contigus. 

Une arène, de type _MemoryArena_ (dans memory/memory.hpp) est une classe template hautement paramétrable qui prend possession d'un bloc mémoire, implémente une stratégie particulière d'allocation (linear, stack, pool...) sur ce bloc, et réagit à une primitive de synchronisation particulière dans un contexte multi-thread. Plusieurs politiques de debugging peuvent être utilisées pour assurer l'intégrité des données (*bounds checking*), la validité des accès (*memory tagging*) et la détection des fuites (*memory tracking*) :

```cpp
template <typename AllocatorT, 
          typename ThreadPolicyT=policy::SingleThread,
          typename BoundsCheckerT=policy::NoBoundsChecking,
          typename MemoryTaggerT=policy::NoMemoryTagging,
          typename MemoryTrackerT=policy::NoMemoryTracking>
class MemoryArena
{
    // ...
};
```

Pour instancier une arène on procède comme suit :
```cpp
typedef memory::MemoryArena<memory::LinearAllocator, 
                            memory::policy::SingleThread, 
                            memory::policy::SimpleBoundsChecking,
                            memory::policy::NoMemoryTagging,
                            memory::policy::SimpleMemoryTracking> LinArena;

memory::HeapArea area(10_MB);
LinArena arena(area.require_block(1_MB));
```
Ici, on alloue tout d'abord un gros bloc mémoire de 10MB sur le tas grâce à une surface _memory::HeapArea_, puis on récupère un intervalle de pointeurs sur un sous-block de 1MB via HeapArea::require_block(size_t) que l'on affecte à l'arène. Noter l'utilisation des littéraux pour désigner les tailles mémoires : ils assurent que la taille retournée est un multiple de 1024, par exemple, 1_MB vaut 1024 * 1024 octets. Les littéraux suivants sont disponibles :
```cpp
    size_t one_byte     = 1_B;
    size_t one_kilobyte = 1_kB;
    size_t one_megabyte = 1_MB;
    size_t one_gigabyte = 1_GB;
```


Pour être tout à fait complet, voici la fonction de memory/memory_utils.h que j'utilise pour calculer l'adresse alignée la plus proche :
```cpp
static inline std::size_t alignment_padding(std::size_t base_address, std::size_t alignment)
{
    std::size_t multiplier = (base_address / alignment) + 1;
    std::size_t aligned_address = multiplier * alignment;
    std::size_t padding = aligned_address - base_address;
    
    return padding;
}
```
La valeur "padding" retournée est le nombre d'octets de padding avant l'adresse alignée.

Un ensemble de macros a été codé afin de conserver une syntaxe relativement proche de celle des opérateurs new, new[], delete et delete[], mais en spécifiant l'arène concernée par l'opération. Une arène peut allouer un bloc d'une taille donnée, et forcer un alignement particulier du pointeur retourné à l'utilisateur :
```cpp
Object* instance = W_NEW(Object, arena);
W_DELETE(instance, arena);

instance = W_NEW_ALIGN(Object, arena, 16);
W_DELETE(instance, arena);
```

Il est aussi possible d'allouer des tableaux d'objets, de manière statique ou dynamique :

```cpp
Object* obj_array = W_NEW_ARRAY(Object[10], arena);
W_DELETE_ARRAY(obj_array, arena);

obj_array = W_NEW_ARRAY_ALIGN(Object[10], arena, 32);
W_DELETE_ARRAY(obj_array, arena);

int num_objects = // ...
obj_array = W_NEW_ARRAY_DYNAMIC(Object, num_objects, arena);
W_DELETE_ARRAY(obj_array, arena);

obj_array = W_NEW_ARRAY_DYNAMIC_ALIGN(Object, num_objects, arena, 64);
W_DELETE_ARRAY(obj_array, arena);
```
Ce qui se passe dans les coulisses est assez fascinant. Les différentes macros ont pour objectif de remplacer les opérateurs *new*, *new[]*, *delete* et *delete[]* dans le contexte d'une arène, et donc il faut reproduire leurs comportements. [1] est une source excellente pour en comprendre le fonctionnement détaillé. De manière très schématique :

    * new alloue un espace mémoire de taille suffisante pour un objet de type T,
      appèle le constructeur T() (si T n'est pas POD) et retourne un pointeur
      de type T* à l'utilisateur vers le début de l'espace alloué.
    * delete appèle le destructeur ~T() (si T est non-POD) et libère la mémoire.
    * new[] réserve 4 + N*sizeof(T) octets, sauvegarde le nombre N d'instances à 
      générer sur les 4 premiers octets, puis appèle T() itérativement sur chaque
      slot réservé (si T non-POD) avant de retourner un pointeur T* vers la première
      instance (et NON vers le début de l'allocation).
    * delete[] remonte de 4 octets avant le pointeur qui lui est passé pour lire
      le nombre d'instances à détruire, puis appèle ~T() itérativement pour 
      chaque instance (si T non-POD), avant de libérer l'ensemble de la mémoire.

Essentiellement, il y a deux points cruciaux qui ont guidé la mise en oeuvre :

    * new et new[] n'ont pas besoin d'appeler de constructeur pour les types POD, de même
      que delete et delete[] n'ont pas besoin d'appeler de destructeurs pour ces mêmes
      types, ce qui laisse la voie à une optimisation, puisque la nature POD ou non
      d'un type est connaissable en compile-time via les type traits.
    * Pour un type non-POD, new[] doit faire du bookkeeping afin que delete[] puisse 
      fonctionner.

Une arène possède une fonction allocate() et une fonction deallocate() qui réalisent les allocations / désallocations au gré des politiques qui la paramétrisent. Les macros précédentes appèlent des classes templates intermédiaires (_NewArray_, _DeleteArray_ et _Delete_) qui réalisent les allocations / désallocations nécessaires en appelant ces fonctions de l'arène. Le prototype de allocate() est le suivant :
```cpp
    void* allocate(size_t size, size_t alignment, size_t offset, const char* file, int line);
```
Le paramètre "offset" est essentiellement utilisé pour l'allocation de tableaux de types non-POD, afin de pouvoir écrire le nombre d'instances sur 4 octets avant le tableau, mais néanmoins retourner un pointeur sur le début du tableau à l'utilisateur. Les paramètres "file" et "line" sont prévus pour recevoir l'évaluation des macros __FILE__ et __LINE__, ce qui renseigne le memory tracker sur lieu de l'allocation.

###LinearAllocator

###HexDump bien classe
memory/memory_utils.h propose une fonction hex_dump() qui réalise comme son nom l'indique un dump mémoire à une adresse donnée, sur une taille donnée. Il est possible de configurer le dumper via hex_dump_highlight() pour surligner d'une couleur particulière un certain motif sur 4 octets. 

Exemple d'utilisation :
```cpp
    memory::hex_dump_highlight(0xf0f0f0f0, WCB(100,0,0));
    memory::hex_dump_highlight(0x0f0f0f0f, WCB(0,0,100));
    memory::hex_dump_highlight(0xd0d0d0d0, WCB(200,100,0));
    memory::hex_dump(std::cout, arena.get_allocator().begin(), size, "HEX DUMP");
```
![Hex dump.\label{figHexDump}](../../Erwin_rel/screens_erwin/erwin_5a_hexdump.png)

Je prévois de lui coder un petit mode ASCII à l'occasion, et pourquoi pas de pousser un peu la détection de motifs. Cet outil m'a déjà été d'une aide inestimable.

###Sources:
    [1] https://blog.molecular-matters.com/2011/07/05/memory-system-part-1/
    [2] https://blog.molecular-matters.com/2011/07/07/memory-system-part-2/
    [3] https://blog.molecular-matters.com/2011/07/08/memory-system-part-3/
    [4] https://blog.molecular-matters.com/2011/07/15/memory-system-part-4/
    [5] https://blog.molecular-matters.com/2011/08/03/memory-system-part-5/
    [6] https://blog.molecular-matters.com/2012/08/14/memory-allocation
        -strategies-a-linear-allocator/
    [7] https://blog.molecular-matters.com/2012/09/17/memory-allocation
        -strategies-a-pool-allocator/
    [8] https://blog.molecular-matters.com/2015/02/13/stateless-layered
        -multi-threaded-rendering-part-4-memory-management-synchronization/



###Sources:
    [1] http://realtimecollisiondetection.net/blog/?p=86