#[Ce qui doit changer, ce qu'on doit garder]

##Pipeline
on change:
    * tout.

On a un seul "renderer" : le _RenderThread_. Cet objet peut pousser des commandes _RenderCommand_ dans une queue _RenderQueue_. Une clé est calculée pour chaque commande en fonction des ressources qu'elle implique (material, ...) et en fin de frame, la queue est triée via ces clés. Il en résulte une série de draw commands minimisant le state change (optimisée) que le _RenderThread_ peut pousser vers le GPU en async, alors que la frame d'après est calculée.

Les _Layers_ sont des objets empilables dans une _LayerStack_ possédée par l'_Application_, dont la fonction run() ne fait qu'itérer les layers dans l'ordre de soumission en appelant leur fonction update(). Cette fonction update est responsable de la soumission des _DrawCommand_ à la _RenderQueue_.

Quand tous les layers ont été itérés, la queue est triée et dispatchée via RenderThread::flush(), et (à travers l'indirection _GraphicsContext_) on a un call vers swap_buffers().

##Shaders

on change:
    * _Shader_ est une interface API-agnostic du moteur de rendu, et masque des implémentations API-specific
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

![Exemple d'output du logger.\label{figLogger}](../Erwin_rel/screens_erwin/erwin_0d_logger.png)

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
    [ ] Gérer les includes.
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

![Application Sandbox utilisant le batch renderer.\label{figBatchRendering}](../Erwin_rel/screens_erwin/erwin_0a_2d_batch_rendering.png)

##2D Instance Renderer
Le deuxième renderer que j'ai codé utilise une approche différente : l'instanciation. _InstanceRenderer2D_ fonctionne sur le même genre de dynamique que le précédent, mais crée un SSBO par batch. A chaque appel à draw_quad, les données en argument (la position, l'échelle et la couleur) sont simplement empilées dans un vector de struct _InstanceData_. Quand ce vector est plein, il est déchargé dans un SSBO via la fonction map(). Le batch suivant est créé, rempli, déchargé dans un autre SSBO etc.
Lors de end_scene(), pour chaque SSBO qu'on attache au shader, un SEUL quad (contenu dans un vertex buffer) est soumis au _RenderDevice_ via draw_indexed_instanced() (qui en sous-main appèle glDrawElementsInstanced()).
Dans le shader on peut accéder aux données par-instance via l'indice gl_InstanceID.

J'ai eu une grosse emmerde dans un premier temps (affichage décalé / buggé), que j'ai bien mis 30 minutes à piger. En remarquant que le premier quad était toujours bien dessiné mais jamais les suivants, j'ai compris que **l'alignement des données dans le layout du SSBO est méga important** C'est la raison pour laquelle le membre color est un vec4 et non un vec3, pour que la taille totale d'une struct tienne sur 32 bytes (padding).

![Application Sandbox utilisant l'instanced renderer.\label{figInstancedRendering}](../Erwin_rel/screens_erwin/erwin_0b_2d_instanced_rendering.png)

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

![FudgePacker et les atlas qu'il génère.\label{figFudgePacker}](../Erwin_rel/screens_erwin/erwin_2a_fudge_packer.png)

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

![Un rendu texturé (tiled rendering) depuis un atlas.\label{figAtlasRendering}](../Erwin_rel/screens_erwin/erwin_3b_atlas_2d_rendering.png)

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

    TODO

![Meanwhile somewhere in Mandelbrot world...\label{figFractal}](../Erwin_rel/screens_erwin/erwin_1c_fractal_explorer.png)


###Sources:
    [1] https://www.youtube.com/watch?v=zmWkhlocBRY