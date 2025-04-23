
/*

    Code par Matéo Toillion
    Dernièrement modifié le 12/04/2025
    
    Pour visualiser et explorer un Mandelbrot

*/

#define SDL_MAIN_HANDLED

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

// Pour la librairie graphique SDL
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL_ttf.h>


// Pour les calculs de haute précisions
// Seulement pour la version linux
#ifdef __linux__
    #include <mpfr.h>
    #include <gmp.h>
#endif


// Etats vrais ou faux
#define false 0
#define true 1

// Type des booléens
#define bool uint8_t

// Retourne la valeur la plus petite ou la plus grande des deux
#define smallest(a, b) ((a > b) ? (b) : (a))
#define largest(a, b)  ((a < b) ? (b) : (a))

// Définit le nombre de fois ou on peut revenir en arrière
#define MAX_HISTORY 1000

// La palette de couleur que va utiliser le Mandelbrot
#define PALETTE_SIZE 256


// Structure de la pile d'historique des zooms
typedef struct {
    double zoom;
    double offsetX;
    double offsetY;
} FractalView;


// Les différents types de fractales
typedef enum {
    FRACTAL_MANDELBROT,
    FRACTAL_JULIA,
    FRACTAL_BURNING_SHIP,
    FRACTAL_TRICORN,
    FRACTAL_MULTIBROT
} FractalType;

// Pour la séparation en un deuxième thread lors du calcul
typedef struct {
    int *iterationMap;
    int *iterationMapBis;
    int max_iteration;
    double zoom, offsetX, offsetY;
    int width, height;
    bool antialiasing;

    int *progress;  // De 0 à 100
    bool *finished;  // Retour du thread pour indiquer qu'il a terminé
    
    bool shouldStop;  // Pour signaler au thread qu'il doit s'arrêter
    
    #ifdef __linux__
    bool highPrecision;  // Précise si on utilise les fonction haute précision ou non
    #endif
    
    FractalType fractalType;  // Type de fractale
    double julia_c_re, julia_c_im; // Pour Julia
    int multibrot_power;           // Pour Multibrot
} FractalTask;


// Pour mieux voir les différents types de menus
enum menuTypes {
    no_menu = 0,
    max_iteration_menu = 1,
    zoom_menu = 2,
    offsetX_menu = 3,
    offsetY_menu = 4
};


// Pour les différents types de positionnement du texte
typedef enum {
    ORIGIN_UP_LEFT,
    ORIGIN_UP_CENTER,
    ORIGIN_UP_RIGHT,
    ORIGIN_MIDDLE_LEFT,
    ORIGIN_MIDDLE_CENTER,
    ORIGIN_MIDDLE_RIGHT,
    ORIGIN_DOWN_LEFT,
    ORIGIN_DOWN_CENTER,
    ORIGIN_DOWN_RIGHT
} OriginType;


// Pour les différentes palettes de couleurs
typedef enum {
    NO_COLOR = 0,
    HOT_COLD = 1,
    WHITE_BLACK = 2,
    RAINBOW = 3
} ColorSchemes;






// Chaine de pointeurs pour la liste de textures
typedef struct FractalList {
    SDL_Texture* texture;
    int* iterationMap;
    struct FractalList* next;
    struct FractalList* prev;
    double zoom, logZoom, offsetX, offsetY;
    int width, height;
    int max_iterations;
} FractalList;



// Pour récupérer les données de la police d'écriture
extern unsigned char DejaVuSans_ttf[];
extern unsigned int DejaVuSans_ttf_len;



// Charge une police d'écriture depuis la mémoire (un fichier .c)
TTF_Font* load_font_from_memory(int size);

// Gestion des palette de couleurs du Mandelbrot
void generate_palette_hot_cold(SDL_Color palette[PALETTE_SIZE]);
void generate_palette_white_black(SDL_Color palette[PALETTE_SIZE]);
void generate_palette_rainbow(SDL_Color palette[PALETTE_SIZE]);

// Gestion de l'historique de position de l'image
void push_view(FractalView history[MAX_HISTORY], int* historyIndex, double zoom, double offsetX, double offsetY);
bool pop_view(FractalView history[MAX_HISTORY], int* historyIndex, double *zoom, double *offsetX, double *offsetY);

// Gestion création et suppression de texture de la liste
FractalList* push_texture(FractalList** head, double zoom, double offsetX, double offsetY, int *number, int* length);
FractalList* pop_texture(FractalList** head, FractalList* toRemove, int* length);

// Dessine le texte passé en paramètre
void render_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, OriginType origin);
// Dessine une barre de progression ainsi qu'un texte
void draw_loading_bar(SDL_Renderer* renderer, TTF_Font* font, char* text, int progress, int width, int height);

// Affiche un double de facon normale ou scientifique en fonction de sa taille
void format_float_auto(char* buffer, size_t size, double value);
// Affichage dynamique en fonction du zoom
void format_float_zoom_adaptive(char* buffer, size_t buffer_size, double value, double zoom);

// Calcul des positions sur l'écran par rapport au positions dans la fractale
void screen_to_fractal(int x, int y, double zoom, double offsetX, double offsetY, int width, int height, double *fx, double *fy);
// Renvoie la valeur passée, limitée au minimum et au maximum passé
double clamp_double(double val, double min, double max);

// Calcul de l'image du Mandelbrot en fonction des paramètres
int calculate_iterations(void* arg);

// Calcul de l'image du mandelbrot pour les différents types de fractales
void calculate_iterations_mandelbrot(FractalTask* task);
void calculate_iterations_julia(FractalTask* task);
void calculate_iterations_burning_ship(FractalTask* task);
void calculate_iterations_tricorn(FractalTask* task);
void calculate_iterations_multibrot(FractalTask* task);
void calculate_iterations_mandelbrot(FractalTask* task);
// Calcul de mandelbrot utilisant les bibliothèques mpfr et gmp pour une précision théoriquement infinie
#ifdef __linux__
void calculate_iterations_mandelbrot_mpfr(FractalTask* task);
void calculate_iterations_julia_mpfr(FractalTask* task);
void calculate_iterations_burning_ship_mpfr(FractalTask* task);
void calculate_iterations_tricorn_mpfr(FractalTask* task);
void calculate_iterations_multibrot_mpfr(FractalTask* task);
void calculate_iterations_mandelbrot_mpfr(FractalTask* task);
#endif

// Rendu des itérations en une image
void render_iterations(SDL_Renderer *renderer, int *iterationMap, int w, int h, SDL_Color *palette, int max_iteration, bool antialiasing);

// Dessine la texture du Mandelbrot en prenant une partie d'une texture, et la collant sur une partie d'une autre texture
int draw_all_textures(SDL_Renderer *renderer, int windowWidth, int windowHeight, double zoom, double offsetX, double offsetY, 
                      FractalList* texturesList, FractalList* selectedTexture, bool selectTextureOn);






int main(int argc, char *argv[]) {


     /* * * * * * * * * */
    /* Choix utilisateur */
    
    // Indique si on cache le texte ou non
    bool hideInterface = false;
    
    // Indique si on veut afficher un bord blanc autour des textures
    bool selectTextureOn = false;
    
    // Active ou non l'antialiasing du Mandelbrot
    bool activateAntialiasing = true;
    
    // Définit si le calcul du mandelbrot sera précis ou normal
    // Seulement dans la version linux
    #ifdef __linux__
    bool advancedMode = false;
    #endif
    
    // Donne le nombre d'itérations jusqu'a lequel on va a chaque calcul de pixel du mandelbrot
    int max_iteration = 200;

    // Valeur de zoom et d'offset par défaut (position de départ)
    double zoom = 200.0;
    double offsetX = -0.5;
    double offsetY = 0.0;
    
    // Définit les couleurs utilisées
    int colorScheme = HOT_COLD;
    
    // Définit l'image sélectionnée dans la liste
    FractalList* selectedImage = NULL;



     /* * * * * */
    /* Programme */

    // Taille de la fenêtre de départ
    int windowWidth = 1200;
    int windowHeight = 800;
    
    // Taille minimale de la fenêtre
    const int minWindowWidth = 500;
    const int minWindowHeight = 400;
    
    // Nom de la fenêtre
    const char windowName[] = "Fractale Mandelbrot";

    // Chemin police d'écriture
    //const char fontPath[] = "DejaVuSans.ttf";

    // Si est à 0, on sort du programme
    bool running = true;
    
    // Vrai à la première éxécution de la boucle
    bool firstExecution = true;
    
    // Valeur ou on enregistre les derniers zooms et déplacements
    double lastZoom = zoom;
    double lastOffsetX = offsetX;
    double lastOffsetY = offsetY;
    
    // Variables permettant de suivre les demandes de dessin
    bool redrawInterface = false;
    bool calculateImage = true;
    bool drawingMade = false;
    bool redrawImage = false;
    bool redrawLoading = false;
    
    // Après modification de taille de la fenêtre, indique si on attend le clic de déblocage
    bool initialClickDone = true;
    
    // Indique le menu actuel ouvert (1: sélection précision normale/précise. 2: Entrée du nombre maximal d'itérations)
    uint8_t menuMode = 0;
    
    // Buffer de texte d'entrée et de sortie
    char inputBuffer[100];
    bool inputStringModified = false;
    char displayBuffer[160];
    
    // Retient le type de la dernière action utilisateur, sert a limiter le nombre de choses qu'on met dans l'historique
    int lastActionType = 0;
    int lastActionValue = 0;

    // Gestion du glissement avec le clic
    bool leftDragging = false;
    bool leftSelecting = false;
    int leftClickStartX = 0, leftClickStartY = 0;
    bool rightSelecting = false;
    bool rightDragging = false;
    SDL_Point selectStart = {0, 0}, selectEnd = {0, 0};
    
    // Si un calcul de fractale est en cours
    bool fractalCalcPending = false;
    
    // Variables décrivant si on soit transformer les tables d'itérations en imges, et comment
    bool renderIteration = false;
    bool renderAllIterations = false;
    bool renderAllIterationsWithReallocate = false;
    
    // Si on doit ajouter l'image générée à la liste
    bool newIterationsCalculated = false;
    
    
    // Progrès actuel du calcul en pourcentage
    int progress = 0;
    // Dernier progrès, pour actualisation de l'affichage
    int lastProgress = 0;
    // Définit si le calcul est terminé ou non
    bool finished = false;
    
    
    // La liste de textures calculées du fractal
    FractalList* fractalList = NULL;
    int fractalListLength = 0;
    FractalList* newFractalElement = NULL;
    
    
    // La tache passée au second thread de calcul
    FractalTask task;
    task.iterationMap = malloc(windowWidth * windowHeight * sizeof(int));
    task.iterationMapBis = malloc(windowWidth * windowHeight * sizeof(int));
    if (task.iterationMap == NULL || task.iterationMapBis == NULL) {
        SDL_Log("Erreur d'allocation mémoire pour la map des itérations.");
        exit(EXIT_FAILURE);
    }
    task.max_iteration = 0;
    task.zoom = 0;
    task.offsetX = 0;
    task.offsetY = 0;
    task.width = 0;
    task.height = 0;
    task.antialiasing = false;
    task.progress = &progress;
    task.finished = &finished;
    task.shouldStop = false;
    #ifdef __linux__
    task.highPrecision = false;
    #endif
    task.fractalType = 0;
    task.julia_c_re = 0;
    task.julia_c_im = 0;
    task.multibrot_power = 0;

    // le pointeur vers le thread de calcul en cours
    SDL_Thread* currentCalcThread = NULL;


    // La palette de couleur que va utiliser le Mandelbrot
    SDL_Color palette[PALETTE_SIZE];
    // Génère la palette de couleurs qui va servir à colorer le mandelbrot
    switch (colorScheme) {
        case HOT_COLD:
            generate_palette_hot_cold(palette);
            break;
        case WHITE_BLACK:
            generate_palette_white_black(palette);
            break;
        case RAINBOW:
            generate_palette_rainbow(palette);
    }
    
    
    // Liste de l'historique des zooms
    FractalView history[MAX_HISTORY];
    int historyIndex = -1;
    
    
    // Définit le type de fractale, ainsi que les paramètres des types spéciaux
    FractalType fractalType = FRACTAL_MANDELBROT;
    double julia_c_re = -0.7, julia_c_im = 0.27015; // Pour Julia
    int multibrot_power = 3;                        // Pour Multibrot

    
    // Initialise la police d'écriture
    TTF_Init();
    TTF_Font *font = load_font_from_memory((int)(8 + windowWidth * 0.006));
    if (!font) {
        SDL_Log("Erreur chargement police d'écriture : %s", TTF_GetError());
        exit(EXIT_FAILURE);
    }

    // Initialise le moteur graphique et la fenêtre
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow(windowName, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, SDL_WINDOW_RESIZABLE);
    // Taille minimale de la fenêtre
    SDL_SetWindowMinimumSize(window, minWindowWidth, minWindowHeight);
    
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // Ce qui va contenir tout la texture de la actuellement générée
    SDL_Texture *fractalTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, windowWidth, windowHeight);
    if (fractalTexture == NULL) {
        SDL_Log("Erreur lors de la création de la texture initiale : %s", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    
    // Contient les évenement de la fenêtre
    SDL_Event event;
    
    
    // Boucle principale d'éxécution
    while (running) {
    
        // On y passe tant qu'on a des évenements à traiter
        while (SDL_PollEvent(&event)) {
        
            static bool openingMenu = false;
        
            // Si on ferme la page
            if (event.type == SDL_QUIT) {
                task.shouldStop = true;
                running = false;
            }
            // Evenement lorsqu'on change la taille de la fenêtre
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED && !firstExecution) {
                windowWidth = event.window.data1;
                windowHeight = event.window.data2;
                bool newWindowOutOfBounds = false;

                // Appliquer les limites minimales manuellement au cas ou la limite placée ne fonctionne pas
                if (windowWidth < minWindowWidth) {
                    windowWidth = minWindowWidth;
                    newWindowOutOfBounds = true;
                }
                if (windowHeight < minWindowHeight) {
                    windowHeight = minWindowHeight;
                    newWindowOutOfBounds = true;
                }
                // Redimensionne la fenêtre si la nouvelle dimension est en dehors des limites
                if (newWindowOutOfBounds) {
                    SDL_SetWindowSize(window, windowWidth, windowHeight);
                }

                // Ferme puis réouvre la police d'écriture à la bonne taille pour la nouvelle taille de la fenêtre
                TTF_CloseFont(font);
                font = load_font_from_memory((int)(8 + windowWidth * 0.006));
                if (!font) {
                    SDL_Log("Erreur chargement police d'écriture : %s", TTF_GetError());
                    return 1;
                }
                
                // Ferme tout les menus actifs
                menuMode = no_menu;
                
                initialClickDone = false;
                redrawInterface = true;
            }
            // Revient en arrière dans l'historique sur clic gauche
            if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_RIGHT && !rightDragging && initialClickDone && !menuMode) {
                if (pop_view(history, &historyIndex, &zoom, &offsetX, &offsetY)) {
                    redrawInterface = true;
                }
            }
            // Clic molette ou espace recalcule le mandelbrot
            if (((event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_MIDDLE) || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE)) && !menuMode && initialClickDone) {
                redrawInterface = true;
                calculateImage = true;
            }
            // Si on scrolle avec la molette
            if (event.type == SDL_MOUSEWHEEL && initialClickDone && !menuMode) {

                // 1. Obtenir la vraie position de la souris
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);

                // 2. Position fractale AVANT zoom
                double fx_before, fy_before;
                screen_to_fractal(mouseX, mouseY, zoom, offsetX, offsetY, windowWidth, windowHeight, &fx_before, &fy_before);

                // 3. Mémoriser le type d’action (historique)
                if (event.type != lastActionType) {
                    push_view(history, &historyIndex, zoom, offsetX, offsetY);
                    lastActionType = SDL_MOUSEWHEEL;
                }

                // 4. Modifier le zoom
                if (event.wheel.y > 0) {
                    zoom *= 1.3;
                } else if (event.wheel.y < 0) {
                    zoom /= 1.3;
                }

                // 5. Position fractale APRÈS zoom
                double fx_after, fy_after;
                screen_to_fractal(mouseX, mouseY, zoom, offsetX, offsetY, windowWidth, windowHeight, &fx_after, &fy_after);

                // 6. Calcul du delta de déplacement (on veut rester centré sur la souris)
                offsetX += fx_before - fx_after;
                offsetY += fy_before - fy_after;

                redrawInterface = true;
            }
            if (event.type == SDL_KEYDOWN && initialClickDone && !menuMode) {
                switch (event.key.keysym.sym) {
                    // Flêche haut
                    case SDLK_UP:
                        // Si on vient de changer d'action de mouvement, enregistrer la position dans l'historique
                        if (event.type != lastActionType || event.key.keysym.sym != lastActionValue) {
                            push_view(history, &historyIndex, zoom, offsetX, offsetY);
                            lastActionType = SDL_KEYDOWN;
                            lastActionValue = event.key.keysym.sym;
                        }
                        offsetY -= 80 / zoom;
                        redrawInterface = true;
                        break;
                    // Flêche bas
                    case SDLK_DOWN:
                        // Si on vient de changer d'action de mouvement, enregistrer la position dans l'historique
                        if (event.type != lastActionType || event.key.keysym.sym != lastActionValue) {
                            push_view(history, &historyIndex, zoom, offsetX, offsetY);
                            lastActionType = SDL_KEYDOWN;
                            lastActionValue = event.key.keysym.sym;
                        }
                        offsetY += 80 / zoom;
                        redrawInterface = true;
                        break;
                    // Flêche gauche
                    case SDLK_LEFT:
                        // Si on vient de changer d'action de mouvement, enregistrer la position dans l'historique
                        if (event.type != lastActionType || event.key.keysym.sym != lastActionValue) {
                            push_view(history, &historyIndex, zoom, offsetX, offsetY);
                            lastActionType = SDL_KEYDOWN;
                            lastActionValue = event.key.keysym.sym;
                        }
                        offsetX -= 80 / zoom;
                        redrawInterface = true;
                        break;
                    // Flêche droite
                    case SDLK_RIGHT:
                        // Si on vient de changer d'action de mouvement, enregistrer la position dans l'historique
                        if (event.type != lastActionType || event.key.keysym.sym != lastActionValue) {
                            push_view(history, &historyIndex, zoom, offsetX, offsetY);
                            lastActionType = SDL_KEYDOWN;
                            lastActionValue = event.key.keysym.sym;
                        }
                        offsetX += 80 / zoom;
                        redrawInterface = true;
                        break;
                    // Touche égal (+)
                    case SDLK_EQUALS:
                        // Si on vient de changer d'action de mouvement, enregistrer la position dans l'historique
                        if (event.type != lastActionType || event.key.keysym.sym != lastActionValue) {
                            push_view(history, &historyIndex, zoom, offsetX, offsetY);
                            lastActionType = SDL_KEYDOWN;
                            lastActionValue = event.key.keysym.sym;
                        }
                        zoom *= 1.6;
                        redrawInterface = true;
                        break;
                    // Touche moins
                    case SDLK_MINUS:
                        // Si on vient de changer d'action de mouvement, enregistrer la position dans l'historique
                        if (event.type != lastActionType || event.key.keysym.sym != lastActionValue) {
                            push_view(history, &historyIndex, zoom, offsetX, offsetY);
                            lastActionType = SDL_KEYDOWN;
                            lastActionValue = event.key.keysym.sym;
                        }
                        zoom /= 1.6;
                        redrawInterface = true;
                        break;
                }
            }
            if (event.type == SDL_KEYDOWN && initialClickDone && !menuMode && !fractalCalcPending) {
                switch (event.key.keysym.sym) {
                    case SDLK_i:
                        menuMode = max_iteration_menu;
                        openingMenu = true;
                        break;
                    case SDLK_w:
                        menuMode = zoom_menu;
                        openingMenu = true;
                        break;
                    case SDLK_x:
                        menuMode = offsetX_menu;
                        openingMenu = true;
                        break;
                    case SDLK_c:
                        menuMode = offsetY_menu;
                        openingMenu = true;
                        break;
                    case SDLK_h:
                        // Toggle pour cacher/montrer l'affichage avec la touche H
                        hideInterface = !hideInterface;
                        redrawInterface = true;
                        break;
                    case SDLK_j:
                        // Toggle pour activer/désactiver l'antialiasing avec la touche J
                        activateAntialiasing = !activateAntialiasing;
                        redrawInterface = true;
                        renderAllIterations = true;
                        renderIteration = true;
                        break;
                    case SDLK_l:
                        // Toggle pour activer/désactiver la sélection des textures
                        selectTextureOn = !selectTextureOn;
                        redrawInterface = true;
                        break;
                    #ifdef __linux__
                        case SDLK_m:
                            // Précision complexe seulement dans la version linux avec la touche M
                            advancedMode = !advancedMode;
                            redrawInterface = true;
                            break;
                    #endif
                    case SDLK_b:
                        // Touche B pour parcourir les couleurs de palettes
                        switch (colorScheme) {
                            case HOT_COLD:
                                colorScheme = RAINBOW;
                                generate_palette_white_black(palette);
                                break;
                            case WHITE_BLACK:
                                colorScheme = HOT_COLD;
                                generate_palette_hot_cold(palette);
                                break;
                            case RAINBOW:
                                colorScheme = WHITE_BLACK;
                                generate_palette_rainbow(palette);
                                break;
                        }
                        redrawInterface = true;
                        renderAllIterations = true;
                        renderIteration = true;
                        break;
                    case SDLK_k:
                        // Touche K pour parcourir les couleurs de palettes
                        switch (fractalType) {
                            case FRACTAL_MANDELBROT:
                                fractalType = FRACTAL_JULIA;
                                break;
                            case FRACTAL_JULIA:
                                fractalType = FRACTAL_BURNING_SHIP;
                                break;
                            case FRACTAL_BURNING_SHIP:
                                fractalType = FRACTAL_TRICORN;
                                break;
                            case FRACTAL_TRICORN:
                                fractalType = FRACTAL_MULTIBROT;
                                break;
                            case FRACTAL_MULTIBROT:
                                fractalType = FRACTAL_MANDELBROT;
                                break;
                        }
                        redrawInterface = true;
                        break;
                    // Touche T pour sélectionner la texture précédente
                    case SDLK_t:
                        if (selectedImage != NULL) {
                            if (selectedImage->prev != NULL) {
                            
                                selectedImage = selectedImage->prev;
                                
                                redrawInterface = true;
                            }
                            if (!selectTextureOn) {
                                selectTextureOn = true;
                                redrawInterface = true;
                            }
                        }
                        break;
                    // Touche Y pour sélectionner la texture suivante
                    case SDLK_y:
                        if (selectedImage != NULL) {
                            if (selectedImage->next != NULL) {
                            
                                selectedImage = selectedImage->next;
                                
                                redrawInterface = true;
                            }
                            if (!selectTextureOn) {
                                selectTextureOn = true;
                                redrawInterface = true;
                            }
                        }
                        break;
                    case SDLK_r:
                        if (selectedImage != NULL) {
                        
                            // Si mode sélection pas activé, l'active seulement
                            if (selectTextureOn) {
                            
                                FractalList* newSelectedImage = selectedImage;
                                
                                // Prend par défaut l'image précédente, si pas dispo l'image suivante, et si aucune dispo alors NULL
                                if (selectedImage->prev == NULL && selectedImage->next == NULL) {
                                    newSelectedImage = NULL;
                                } else if (selectedImage->prev == NULL) {
                                    newSelectedImage = selectedImage->next;
                                } else {
                                    newSelectedImage = selectedImage->prev;
                                }
                                
                                // Supprime la texture sélectionnée
                                pop_texture(&fractalList, selectedImage, &fractalListLength);
                                
                                // Sélectionne l'image la plus proche disponible
                                selectedImage = newSelectedImage;
                            }
                            
                            selectTextureOn = true;
                            redrawInterface = true;
                        }
                        break;
                }
                
                if (openingMenu) {
                    rightSelecting = false;
                    leftSelecting = false;

                    redrawImage = true;
                    inputStringModified = true;
                    openingMenu = true;

                    inputBuffer[0] = '\0';
                    SDL_StartTextInput();
                }
            }
            
            // Gestion du clic gauche glissé à l'appui
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT && !menuMode && initialClickDone) {
                leftSelecting = true;
                leftDragging = false;
                leftClickStartX = event.button.x;
                leftClickStartY = event.button.y;
            }
            // Gestion du clic gauche, lorsqu'on lache
            if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT && leftDragging) {
                leftSelecting = false;
                leftDragging = false;
            }
            // Gestion du clic gauche glissé, lorsqu'on bouge
            if (event.type == SDL_MOUSEMOTION && leftSelecting && (event.motion.state & SDL_BUTTON_LMASK)) {
                int dx = event.motion.x - leftClickStartX;
                int dy = event.motion.y - leftClickStartY;

                if (!leftDragging && (abs(dx) > 2 || abs(dy) > 2)) {
                    leftDragging = true; // on considère que c’est un vrai glissement
                    // Sauvegarde dans l'historique au début du drag
                    if (event.type != lastActionType) {
                        push_view(history, &historyIndex, zoom, offsetX, offsetY);
                        lastActionType = event.type;
                    }
                }

                if (leftDragging) {
                    offsetX -= dx / zoom;
                    offsetY -= dy / zoom;
                    leftClickStartX = event.motion.x; // mettre à jour pour les prochains deltas
                    leftClickStartY = event.motion.y;
                    redrawInterface = true;
                }
            }
            
            // Gestion du clic droit glissé à l'appui et le clic droit simple
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT && !menuMode && initialClickDone) {
                rightSelecting = true;
                rightDragging = false;
                selectStart.x = event.button.x;
                selectStart.y = event.button.y;
                selectEnd = selectStart;
            }
            // Gestion du clic droit simple
            if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_RIGHT && rightSelecting && !rightDragging) {
                rightSelecting = false;
            }
            // Gestion du clic droit glissé, lors du mouvement
            if (event.type == SDL_MOUSEMOTION && rightSelecting) {
                selectEnd.x = event.motion.x;
                selectEnd.y = event.motion.y;
                rightDragging = true;
                redrawInterface = true;
            }
            // Gestion du clic droit glissé, lorsqu'on lache
            if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_RIGHT && rightDragging) {
                rightSelecting = false;
                rightDragging = false;
                
                push_view(history, &historyIndex, zoom, offsetX, offsetY);

                int x1 = smallest(selectStart.x, selectEnd.x);
                int x2 = largest(selectStart.x, selectEnd.x);
                int y1 = smallest(selectStart.y, selectEnd.y);
                int y2 = largest(selectStart.y, selectEnd.y);

                if (abs(x2 - x1) > 10 && abs(y2 - y1) > 10) {
                    double fx1, fy1, fx2, fy2;
                    screen_to_fractal(x1, y1, zoom, offsetX, offsetY, windowWidth, windowHeight, &fx1, &fy1);
                    screen_to_fractal(x2, y2, zoom, offsetX, offsetY, windowWidth, windowHeight, &fx2, &fy2);

                    offsetX = (fx1 + fx2) / 2;
                    offsetY = (fy1 + fy2) / 2;
                    zoom *= fmin(windowWidth / (double)(x2 - x1), windowHeight / (double)(y2 - y1));
                }
                
                redrawInterface = true;
            }
            
            // Gestion de l'actualisation de la taille de la texture au changement de taille d'écran
            if (event.type == SDL_MOUSEBUTTONDOWN && (event.button.button == SDL_BUTTON_LEFT || event.button.button == SDL_BUTTON_RIGHT) && !menuMode && !initialClickDone && !fractalCalcPending) {

                // Remet à jour toute les textures de la liste
                renderAllIterations = true;
                renderAllIterationsWithReallocate = true;

                initialClickDone = true;
                redrawInterface = true;
            }
            
            // Pour annuler le chargement d'une image 
            if (fractalCalcPending && event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                task.shouldStop = true;
            }

            // Si le menu d'entrée du nombre max d'itérations est activé, on surveille les inputs liés
            if (menuMode) {
            
                if (event.type == SDL_TEXTINPUT) {
                    if (strlen(inputBuffer) + strlen(event.text.text) < sizeof(inputBuffer) - 1) {
                    
                        for (int i = 0; event.text.text[i]; i++) {
                            char c = event.text.text[i];
                            size_t len = strlen(inputBuffer);

                            bool hasDotOrComma = strchr(inputBuffer, '.') || strchr(inputBuffer, ',');
                            bool hasDigit = false;
                            for (size_t j = 0; j < len; j++) {
                                if (isdigit(inputBuffer[j])) {
                                    hasDigit = true;
                                    break;
                                }
                            }

                            if (isdigit(c)) {
                                inputBuffer[len] = c;
                                inputBuffer[len + 1] = '\0';
                                inputStringModified = true;
                                redrawImage = true;
                            }
                            else if ((c == '.' || c == ',') && !hasDotOrComma && hasDigit) {
                                inputBuffer[len] = (c == ',') ? '.' : c; // Remplace la virgule par un point pour atof
                                inputBuffer[len + 1] = '\0';
                                inputStringModified = true;
                                redrawImage = true;
                            }
                            else if (c == '-' && len == 0) {
                                inputBuffer[0] = '-';
                                inputBuffer[1] = '\0';
                                inputStringModified = true;
                                redrawImage = true;
                            }
                            // Sinon : caractère ignoré
                        }
                    }
                    
                } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RETURN && strlen(inputBuffer) > 0) {

                    // Remplace les virgules résiduelles par des points (sécurité supplémentaire)
                    for (int i = 0; inputBuffer[i]; i++) {
                        if (inputBuffer[i] == ',') {
                            inputBuffer[i] = '.';
                        }
                    }

                    // Conversion sécurisée avec strtod
                    char *endptr;
                    double value = strtod(inputBuffer, &endptr);

                    if (*endptr == '\0') {  // conversion réussie

                        switch (menuMode) {
                            case max_iteration_menu:
                                if ((int)(value + 0.5) >= 1) {
                                    max_iteration = (int)(value + 0.5);  // arrondi au plus proche
                                }
                                break;
                            case zoom_menu:
                                if (value > 0) {
                                    zoom = value;
                                }
                                break;
                            case offsetX_menu:
                                if (value > 0) {
                                    offsetX = value;
                                }
                                break;
                            case offsetY_menu:
                                if (value > 0) {
                                    offsetY = value;
                                }
                                break;
                        }

                        menuMode = no_menu;
                        SDL_StopTextInput();

                        redrawInterface = true;
                    }
                    // Sinon : valeur invalide, on sort sans rien faire
                } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_BACKSPACE && strlen(inputBuffer) > 0) {
                
                    inputBuffer[strlen(inputBuffer) - 1] = '\0';
                    
                    inputStringModified = true;
                    redrawImage = true;
                
                } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                
                    menuMode = no_menu;
                    SDL_StopTextInput();
                    
                    redrawInterface = true;
                
                } else if (!openingMenu) {
                
                    redrawInterface = false;
                    redrawImage = false;
                    calculateImage = false;
                }
                openingMenu = false;
            }
        }

        // Si on est en attente du dessin de la fractale
        if (fractalCalcPending) {
        
            // Pas besoin de mettre à jour l'interface, juste le chargement
            if (redrawInterface) {
                redrawLoading = true;
            }
            // Calcul pas encore terminé, on empêche un nouveau calcul
            if (!finished) {
                calculateImage = false;
            }
            // Si progrès à évolué, actualisé l'affichage
            if (progress != lastProgress) {
                redrawInterface = true;
                redrawLoading = true;
                lastProgress = progress;
            }
            
            // Calcul terminé
            if (finished) {
            
                // Si le calcul n'a pas été aborti prématurément
                if (!task.shouldStop) {
                
                    // On lance le rendu en couleur des calculs
                    renderIteration = true;
                    
                    // On ajoute la texture à la liste des textures
                    newIterationsCalculated = true;
                }
                
                // Réinitialise le pointeur vers le thread de calcul
                currentCalcThread = NULL;
                
                redrawInterface = true;    
                fractalCalcPending = false;  
                redrawLoading = false;     
            }
        }
        
        
        // Avec les itérations calculées, on fait maintenant le rendu en couleur sur la texture
        if (renderIteration) {
        
            // Sélectionne la texture comme cible SDL
            SDL_SetRenderTarget(renderer, fractalTexture);
            
            // Lance la ransformation de la liste d'itérations en couleurs
            render_iterations(renderer, task.iterationMap, task.width, task.height, palette, task.max_iteration, activateAntialiasing);
            
            // Si on fait le rendu après le calcul des itérations
            if (newIterationsCalculated) {

                // Pousse la nouvelle texture dans la liste
                newFractalElement = push_texture(&fractalList, lastZoom, lastOffsetX, lastOffsetY, NULL, &fractalListLength);
                // Si aucune image sélectionnée, on sélectionne celle qui vient d'être créée
                selectedImage = newFractalElement;

                newFractalElement->width = task.width;
                newFractalElement->height = task.height;
                newFractalElement->max_iterations = task.max_iteration;
                
                // On donne en plus de la texture la carte des itération si besoin de recommencer le rendu
                newFractalElement->iterationMap = malloc(newFractalElement->width * task.height * sizeof(int));
                if (newFractalElement->iterationMap == NULL) {
                    SDL_Log("Erreur d'allocation mémoire pour la map des itérations de la nouvelle image.");
                    exit(EXIT_FAILURE);
                }
                memcpy(newFractalElement->iterationMap, task.iterationMap, newFractalElement->width * newFractalElement->height * sizeof(int));
                
                
                newFractalElement->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, newFractalElement->width, newFractalElement->height);
                if (newFractalElement->texture == NULL) {
                    SDL_Log("Erreur lors de la création de la texture dans la liste : %s", SDL_GetError());
                    exit(EXIT_FAILURE);
                }
                
                // Sauvegarde la cible de rendu actuelle
                SDL_Texture* previousTarget = SDL_GetRenderTarget(renderer);

                // Rend vers fractalList->texture
                SDL_SetRenderTarget(renderer, newFractalElement->texture);

                // Copie le contenu de fractalTexture dans fractalList->texture
                SDL_RenderCopy(renderer, fractalTexture, NULL, NULL);

                // Restaure la cible de rendu précédente
                SDL_SetRenderTarget(renderer, previousTarget);
                
                newIterationsCalculated = false;
            }
            
            renderIteration = false;
        }
        
        // Si on doit remettre a jour toute les textures de la liste
        if (renderAllIterations) {
        
            // Sélectionne l'écran comme cible SDL
            SDL_SetRenderTarget(renderer, NULL);
        
            // Boucle qui parcours les éléments de la liste
            int actualElementCounter = 1;
            FractalList* actualFractalList = fractalList;
            while (actualFractalList != NULL) {

                // S'il faut réallouer toute les textures
                if (renderAllIterationsWithReallocate) {
                
                    // Supprime l'ancienne texture
                    SDL_DestroyTexture(actualFractalList->texture);

                    // Recrée une texture de la même taille que l'ancienne
                    actualFractalList->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, actualFractalList->width, actualFractalList->height);
                    if (actualFractalList->texture == NULL) {
                        SDL_Log("Erreur lors de la recréation de la texture dans renderAllIterations : %s", SDL_GetError());
                        exit(EXIT_FAILURE); // ou un return propre
                    }
                }
                
                // Sélectionne la texture comme cible SDL
                SDL_SetRenderTarget(renderer, actualFractalList->texture);

                // Fait le rendu sur la texture
                render_iterations(renderer, actualFractalList->iterationMap, actualFractalList->width, actualFractalList->height, palette, actualFractalList->max_iterations, activateAntialiasing);
                

                // Sélectionne l'écran comme cible SDL
                SDL_SetRenderTarget(renderer, NULL);

                // Efface l'écran en noir
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // noir opaque
                SDL_RenderClear(renderer);
                
                // Dessine toute les textures de la liste, bien placé sur l'écran
                draw_all_textures(renderer, windowWidth, windowHeight, zoom, offsetX, offsetY,
                                  fractalList, selectedImage, selectTextureOn);
                
                // Dessine la barre de chargement avec la progression actuelle
                draw_loading_bar(renderer, font, "Chargement en cours...", (int)(((float)actualElementCounter / (float)fractalListLength) * 100), windowWidth, windowHeight);
                
                // Afficher tout les dessins à l'écran
                SDL_RenderPresent(renderer);
                
                
                // Passe à l'élément suivant
                actualElementCounter++;
                actualFractalList = actualFractalList->next;
            }
            
            renderAllIterations = false;
            renderAllIterationsWithReallocate = false;
        }

        // Si l'utilisateur n'a pas encore cliqué après le redimensionnement et qu'on actualise l'affichage, on affiche just un message
        if (!initialClickDone && redrawInterface) {
            redrawInterface = false;
            redrawImage = true;
            calculateImage = false;
            redrawLoading = true;
        }

        // Si on modifie la vue et qu'on demande un recalcul, ou qu'on force un recalcul
        if (calculateImage && running) {

            // Sélectionne l'écran comme cible            
            SDL_SetRenderTarget(renderer, NULL);
            
            // 1. Sauvegarder l’ancienne texture et sa taille
            int oldW, oldH;
            SDL_QueryTexture(fractalTexture, NULL, NULL, &oldW, &oldH);
            
            // Si taille de la texture plus égale à la taille de la fenêtre, on l'ajuste
            if (oldW != windowWidth || oldH != windowHeight) {

                SDL_Texture* oldTexture = fractalTexture;

                // 2. Créer la nouvelle texture avec la nouvelle taille
                fractalTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, windowWidth, windowHeight);
                if (fractalTexture == NULL) {
                    SDL_Log("Erreur lors de la création de la texture pour redimensionnement fenêtre : %s", SDL_GetError());
                    exit(EXIT_FAILURE);
                }

                // 3. Copier l’ancienne texture dedans, centrée
                SDL_SetRenderTarget(renderer, fractalTexture);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // fond noir si plus grand
                SDL_RenderClear(renderer);

                SDL_Rect srcRect = { 0, 0, oldW, oldH };
                SDL_Rect dstRect = {
                    (windowWidth - oldW) / 2,
                    (windowHeight - oldH) / 2,
                    oldW,
                    oldH
                };

                SDL_RenderCopy(renderer, oldTexture, &srcRect, &dstRect);

                // 4. Nettoyer
                SDL_DestroyTexture(oldTexture);

                // Actualiser la taille de la map d'itérations
                free(task.iterationMap);
                free(task.iterationMapBis);
                task.iterationMap = (int*)malloc(windowWidth * windowHeight * sizeof(int));
                task.iterationMapBis = (int*)malloc(windowWidth * windowHeight * sizeof(int));
                if (task.iterationMap == NULL || task.iterationMapBis == NULL) {
                    SDL_Log("Erreur d'allocation mémoire pour la map des itérations.");
                    exit(EXIT_FAILURE);
                }
            }
            
            // Met à jour les informations passées au second thread de calcul
            task.max_iteration = max_iteration;
            task.zoom = zoom;
            task.offsetX = offsetX;
            task.offsetY = offsetY;
            task.width = windowWidth;
            task.height = windowHeight;
            task.antialiasing = activateAntialiasing;
            task.shouldStop = false;
            #ifdef __linux__
            task.highPrecision = advancedMode;
            #endif
            task.fractalType = fractalType;
            task.julia_c_re = julia_c_re;
            task.julia_c_im = julia_c_im;
            task.multibrot_power = multibrot_power;

            
            // Lance le second thread de calcul
            currentCalcThread = SDL_CreateThread(calculate_iterations, "CalcFractalThread", &task);
            

            // On déclare que le calcul est en cours
            fractalCalcPending = true;
            lastProgress = 1000;
            
            // Sauvegarde les dernière valeurs de zoom et d'offset
            lastZoom = zoom;
            lastOffsetX = offsetX;
            lastOffsetY = offsetY;
        }


        // Si on a une demande de redessin
        if (redrawImage || redrawInterface || calculateImage) {
        
            // Sélectionne l'écran comme cible SDL
            SDL_SetRenderTarget(renderer, NULL);

            // Efface l'écran en noir
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // noir opaque
            SDL_RenderClear(renderer);
            
            // Dessine toute les textures de la liste, bien placé sur l'écran
            draw_all_textures(renderer, windowWidth, windowHeight, zoom, offsetX, offsetY,
                              fractalList, selectedImage, selectTextureOn);

            drawingMade = true;
        }


        // Pour redessiner l'interface
        if ((redrawInterface || calculateImage) && !hideInterface && drawingMade) {

            // Sélectionne l'écran comme cible
            SDL_SetRenderTarget(renderer, NULL);

            // Calculer l'espacement vertical proportionnel à la hauteur de la fenêtre
            float verticalSpacing = 15 + windowWidth * 0.01f; // Par exemple, 3% de la hauteur de la fenêtre

            /* Coté bas gauche de l'écran */

            // Paramètres de l'image, bord bas gauche
            sprintf(displayBuffer, "Nombre d'images actuel: %d", fractalListLength);
            render_text(renderer, font, displayBuffer, 10, windowHeight - 1 * verticalSpacing, ORIGIN_UP_LEFT);
            sprintf(displayBuffer, "Nombre d'itérations max: %d", max_iteration);
            render_text(renderer, font, displayBuffer, 10, windowHeight - 2 * verticalSpacing, ORIGIN_UP_LEFT);
            
            // Pour l'affichage normal ou scientifique des valeurs
            char zoomBuffer[64], offsetXBuffer[64], offsetYBuffer[64];

            format_float_auto(zoomBuffer, sizeof(zoomBuffer), zoom);
            format_float_zoom_adaptive(offsetXBuffer, sizeof(offsetXBuffer), offsetX, zoom);
            format_float_zoom_adaptive(offsetYBuffer, sizeof(offsetYBuffer), offsetY, zoom);

            snprintf(displayBuffer, sizeof(displayBuffer), "Zoom actuel: %s", zoomBuffer);
            render_text(renderer, font, displayBuffer, 10, windowHeight - 3 * verticalSpacing, ORIGIN_UP_LEFT);

            snprintf(displayBuffer, sizeof(displayBuffer), "Offset horizontal: %s", offsetXBuffer);
            render_text(renderer, font, displayBuffer, 10, windowHeight - 4 * verticalSpacing, ORIGIN_UP_LEFT);
            
            snprintf(displayBuffer, sizeof(displayBuffer), "Offset vertical: %s", offsetYBuffer);
            render_text(renderer, font, displayBuffer, 10, windowHeight - 5 * verticalSpacing, ORIGIN_UP_LEFT);

            switch (fractalType) {
                case FRACTAL_MANDELBROT:
                    render_text(renderer, font, "Type de fractale: MANDELBROT", 10, windowHeight - 6 * verticalSpacing, ORIGIN_UP_LEFT);
                    break;
                case FRACTAL_JULIA:
                    render_text(renderer, font, "Type de fractale: JULIA", 10, windowHeight - 6 * verticalSpacing, ORIGIN_UP_LEFT);
                    break;
                case FRACTAL_BURNING_SHIP:
                    render_text(renderer, font, "Type de fractale: BURNING SHIP", 10, windowHeight - 6 * verticalSpacing, ORIGIN_UP_LEFT);
                    break;
                case FRACTAL_TRICORN:
                    render_text(renderer, font, "Type de fractale: TRICORN", 10, windowHeight - 6 * verticalSpacing, ORIGIN_UP_LEFT);
                    break;
                case FRACTAL_MULTIBROT:
                    render_text(renderer, font, "Type de fractale: MULTIBROT", 10, windowHeight - 6 * verticalSpacing, ORIGIN_UP_LEFT);
                    break;
            }
            
            
            /* Coté bas droite de l'écran */

            // Controles, bord bas droite
            if (activateAntialiasing) {
                render_text(renderer, font, "J: Antialiasing - ON", windowWidth - 10, windowHeight - 1 * verticalSpacing, ORIGIN_UP_RIGHT);
            } else {
                render_text(renderer, font, "J: Antialiasing - OFF", windowWidth - 10, windowHeight - 1 * verticalSpacing, ORIGIN_UP_RIGHT);
            }
            
            if (selectTextureOn) {
                render_text(renderer, font, "L: Mode sélection - ON", windowWidth - 10, windowHeight - 2 * verticalSpacing, ORIGIN_UP_RIGHT);
            } else {
                render_text(renderer, font, "L: Mode sélection - OFF", windowWidth - 10, windowHeight - 2 * verticalSpacing, ORIGIN_UP_RIGHT);
            }

            switch (colorScheme) {
                case HOT_COLD:
                    render_text(renderer, font, "B: Couleur - CHAUD/FROID", windowWidth - 10, windowHeight - 3 * verticalSpacing, ORIGIN_UP_RIGHT);
                    break;
                case WHITE_BLACK:
                    render_text(renderer, font, "B: Couleur - BLANC/NOIR", windowWidth - 10, windowHeight - 3 * verticalSpacing, ORIGIN_UP_RIGHT);
                    break;
                case RAINBOW:
                    render_text(renderer, font, "B: Couleur - ARC-EN-CIEL", windowWidth - 10, windowHeight - 3 * verticalSpacing, ORIGIN_UP_RIGHT);
                    break;
            }
            
            render_text(renderer, font, "Clic droit/flèches directionnelles: Déplacer", windowWidth - 10, windowHeight - 4 * verticalSpacing, ORIGIN_UP_RIGHT);
            render_text(renderer, font, "Molette/clic gauche glissé/+&-: Zoomer/Dézoomer", windowWidth - 10, windowHeight - 5 * verticalSpacing, ORIGIN_UP_RIGHT);
            render_text(renderer, font, "Espace/Clic molette: Calculer image", windowWidth - 10, windowHeight - 6 * verticalSpacing, ORIGIN_UP_RIGHT);
            render_text(renderer, font, "H: Interface | Clic droit simple: Retour arrière", windowWidth - 10, windowHeight - 7 * verticalSpacing, ORIGIN_UP_RIGHT);
            render_text(renderer, font, "T/Y: Sélectionner texture | R: Supprimer texture", windowWidth - 10, windowHeight - 8 * verticalSpacing, ORIGIN_UP_RIGHT);
            render_text(renderer, font, "W: Zoom  X: OffsetX  C: OffsetY  I: Itération max", windowWidth - 10, windowHeight - 9 * verticalSpacing, ORIGIN_UP_RIGHT);
            render_text(renderer, font, "K: Type Fractale | P: Paramètres Fractale", windowWidth - 10, windowHeight - 10 * verticalSpacing, ORIGIN_UP_RIGHT);

            #ifdef __linux__
                if (advancedMode) {
                    render_text(renderer, font, "M: Précision Normale/Haute:   HAUTE", windowWidth - 10, windowHeight - 11 * verticalSpacing, ORIGIN_UP_RIGHT);
                } else {
                    render_text(renderer, font, "M: Précision Normale/Haute: NORMALE", windowWidth - 10, windowHeight - 11 * verticalSpacing, ORIGIN_UP_RIGHT);
                }
            #endif

            // Si on sélectionne une zone
            if (rightDragging) {

                // Dessine le rectangle blanc de sélection
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_Rect rect;
                rect.x = smallest(selectStart.x, selectEnd.x);
                rect.y = smallest(selectStart.y, selectEnd.y);
                rect.w = abs(selectEnd.x - selectStart.x);
                rect.h = abs(selectEnd.y - selectStart.y);
                SDL_RenderDrawRect(renderer, &rect);
            }

            drawingMade = true;
        }
        
        // Si on est en train de calculer la prochaine image, on affiche un chargement a l'écran
        if (redrawLoading && fractalCalcPending) {
        
            // Sélectionne l'écran comme cible
            SDL_SetRenderTarget(renderer, NULL);
        
            draw_loading_bar(renderer, font, "Chargement en cours...", progress, windowWidth, windowHeight);
            
            render_text(renderer, font, "Echap pour Annuler", windowWidth / 50, windowHeight / 50, ORIGIN_UP_LEFT);

            redrawLoading = false;
            drawingMade = true;
        }
        
        // Si on attend que l'utilisate clique dans la fenêtre après un changement de taille
        if (redrawLoading && !initialClickDone) {

            // Sélectionne l'écran comme cible
            SDL_SetRenderTarget(renderer, NULL);
            
            render_text(renderer, font, "Cliquez pour réactiver...", windowWidth / 2, windowHeight / 2, ORIGIN_MIDDLE_CENTER);
            
            redrawLoading = false;
            drawingMade = true;
        }
        
        // Si chaine de caractères modifiée, on redessine avec le texte à jour
        if (inputStringModified && menuMode) {

            // Sélectionne l'écran comme cible
            SDL_SetRenderTarget(renderer, NULL);

            render_text(renderer, font, "Echap pour Annuler", windowWidth / 50, windowHeight / 50, ORIGIN_UP_LEFT);

            switch (menuMode) {
                case max_iteration_menu:
                    sprintf(displayBuffer, "Veuillez entrer le nombre maximal d'itérations: %s", inputBuffer);
                    break;
                case zoom_menu:
                    sprintf(displayBuffer, "Veuillez entrer le niveau de zoom: %s", inputBuffer);
                    break;
                case offsetX_menu:
                    sprintf(displayBuffer, "Veuillez entrer l'offset horizontal: %s", inputBuffer);
                    break;
                case offsetY_menu:
                    sprintf(displayBuffer, "Veuillez entrer l'offset vertical: %s", inputBuffer);
                    break;
                default:
                    displayBuffer[0] = '\0';
                    break;
            }
            render_text(renderer, font, displayBuffer,  windowWidth / 50, (windowHeight / 50) + 15 + windowWidth * 0.01f, ORIGIN_UP_LEFT);
            
            inputStringModified = false;
            
            drawingMade = true;
        }
        
        // Si on a dessiné sur l'écran, tout afficher
        if (drawingMade) {
            SDL_SetRenderTarget(renderer, NULL);
            SDL_RenderPresent(renderer);
            drawingMade = false;
        }
        
        // On réinitialise les demandes de dessin
        redrawInterface = false;
        calculateImage = false;
        redrawImage = false;
        redrawLoading = false;
        
        firstExecution = false;
        
        // Evite enormément de lag en ne consommant pas trop de cycles processeur
        SDL_Delay(10);
    }

    // Vide la listes de textures
    while (fractalList != NULL) {
        fractalList = pop_texture(&fractalList, fractalList, &fractalListLength);
    }
    
    // On attend que le second thread ai bien terminé
    SDL_WaitThread(currentCalcThread, NULL);
    currentCalcThread = NULL;

    // Libère la liste des itérations
    free(task.iterationMap);
    free(task.iterationMapBis);

    // Ferme les polices d'écriture
    TTF_CloseFont(font);
    TTF_Quit();

    // Ferme SDL et libère la mémoire
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return EXIT_SUCCESS;
}




// Charge une police d'écriture depuis la mémoire (un fichier .c)
TTF_Font* load_font_from_memory(int size) {
    SDL_RWops* rw = SDL_RWFromConstMem(DejaVuSans_ttf, DejaVuSans_ttf_len);
    if (!rw) {
        SDL_Log("Erreur SDL_RWFromConstMem: %s", SDL_GetError());
        return NULL;
    }

    TTF_Font* font = TTF_OpenFontRW(rw, 1, size); // '1' = SDL gère la libération de rw
    if (!font) {
        SDL_Log("Erreur TTF_OpenFontRW: %s", TTF_GetError());
    }

    return font;
}





// Génère une palette de couleur allant dans couleurs froides au couleurs chaudes
void generate_palette_hot_cold(SDL_Color palette[PALETTE_SIZE]) {
    for (int i = 0; i < PALETTE_SIZE; i++) {
        float t = (float)i / (PALETTE_SIZE - 1);
        palette[i].r = (int)(9 * (1 - t) * t * t * t * 255);         // rouge
        palette[i].g = (int)(15 * (1 - t) * (1 - t) * t * t * 255);  // vert
        palette[i].b = (int)(8.5 * (1 - t) * (1 - t) * (1 - t) * t * 255); // bleu
    }
}


// Génère une palette de couleur allant dans couleurs de noir à blanc
void generate_palette_white_black(SDL_Color palette[PALETTE_SIZE]) {
    for (int i = 0; i < PALETTE_SIZE; i++) {
        float t = (float)i / (PALETTE_SIZE - 1);
        palette[i].r = (int)(255 * t); // rouge
        palette[i].g = (int)(255 * t); // vert
        palette[i].b = (int)(255 * t); // bleu
    }
}


// Génère une palette de couleurs arc-en-ciel
void generate_palette_rainbow(SDL_Color palette[PALETTE_SIZE]) {
    for (int i = 0; i < PALETTE_SIZE; i++) {
        float t = (float)i / (PALETTE_SIZE - 1);

        // Interpolation des composantes de couleurs pour l'arc-en-ciel
        float r = 0.0, g = 0.0, b = 0.0;

        if (t < 1.0 / 6.0) {
            // Rouge vers Jaune
            r = 1.0;
            g = 6.0 * t;
        } else if (t < 2.0 / 6.0) {
            // Jaune vers Vert
            r = 1.0 - 6.0 * (t - 1.0 / 6.0);
            g = 1.0;
        } else if (t < 3.0 / 6.0) {
            // Vert vers Cyan
            g = 1.0;
            b = 6.0 * (t - 2.0 / 6.0);
        } else if (t < 4.0 / 6.0) {
            // Cyan vers Bleu
            g = 1.0 - 6.0 * (t - 3.0 / 6.0);
            b = 1.0;
        } else if (t < 5.0 / 6.0) {
            // Bleu vers Violet
            r = 6.0 * (t - 4.0 / 6.0);
            b = 1.0;
        } else {
            // Violet vers Rouge
            r = 1.0;
            b = 1.0 - 6.0 * (t - 5.0 / 6.0);
        }

        // Conversion des valeurs RGB (0-1) en entier (0-255)
        palette[i].r = (int)(r * 255);
        palette[i].g = (int)(g * 255);
        palette[i].b = (int)(b * 255);
    }
}


// Ajoute la vue actuelle a l'historique
void push_view(FractalView history[MAX_HISTORY], int* historyIndex, double zoom, double offsetX, double offsetY) {
    if (*historyIndex < MAX_HISTORY - 1) {
        *historyIndex = *historyIndex + 1;
        history[*historyIndex].zoom = zoom;
        history[*historyIndex].offsetX = offsetX;
        history[*historyIndex].offsetY = offsetY;
    }
}


// Retire une vue de l'historique et la mettre dans le zoom et l'offset actuel
bool pop_view(FractalView history[MAX_HISTORY], int* historyIndex, double *zoom, double *offsetX, double *offsetY) {
    if (*historyIndex >= 0) {
        *zoom = history[*historyIndex].zoom;
        *offsetX = history[*historyIndex].offsetX;
        *offsetY = history[*historyIndex].offsetY;
        *historyIndex = *historyIndex - 1;
        return true;
    }
    return false;
}


// Ajouter une texture dans la liste, en fonction de son zoom
FractalList* push_texture(FractalList** head, double zoom, double offsetX, double offsetY, int *number, int* length) {
    FractalList* new_node = (FractalList*)malloc(sizeof(FractalList));
    if (!new_node) {
        SDL_Log("Erreur d'allocation mémoire pour la liste des textures de fractales.");
        exit(EXIT_FAILURE);
    }
    
    if (length != NULL) {
        *length = *length + 1;
    }

    new_node->texture = NULL;
    new_node->iterationMap = NULL;
    new_node->zoom = zoom;
    new_node->offsetX = offsetX;
    new_node->offsetY = offsetY;
    new_node->width = 0;
    new_node->height = 0;
    new_node->max_iterations = 0;
    new_node->next = NULL;
    new_node->prev = NULL;

    int index = 0;

    // Cas 1 : insertion en tête
    if (*head == NULL || zoom < (*head)->zoom) {
        new_node->next = *head;
        if (*head != NULL) {
            (*head)->prev = new_node;
        }
        *head = new_node;
        if (number != NULL) *number = index;
        return new_node;
    }

    // Cas 2 : insertion au bon emplacement
    FractalList* current = *head;
    while (current->next != NULL && current->next->zoom <= zoom) {
        current = current->next;
        index++;
    }

    // Insertion après current
    new_node->next = current->next;
    new_node->prev = current;
    if (current->next != NULL) {
        current->next->prev = new_node;
    }
    current->next = new_node;

    if (number != NULL) *number = index + 1;

    return new_node;
}


// Retire une des textures données de la liste
FractalList* pop_texture(FractalList** head, FractalList* toRemove, int* length) {
    if (toRemove == NULL || head == NULL || *head == NULL) {
        return NULL;
    }
    
    if (length != NULL) {
        *length = *length - 1;
    }

    // Si c'est la tête de liste
    if (toRemove == *head) {
        *head = toRemove->next;
    }

    // Répare les liens
    if (toRemove->prev != NULL) {
        toRemove->prev->next = toRemove->next;
    }

    if (toRemove->next != NULL) {
        toRemove->next->prev = toRemove->prev;
    }

    // Libère les ressources
    if (toRemove->texture != NULL) {
        SDL_DestroyTexture(toRemove->texture);
    }
    if (toRemove->iterationMap != NULL) {
        free(toRemove->iterationMap);
    }

    // Sauvegarde le suivant si on veut continuer à itérer
    FractalList* nextNode = toRemove->next;

    free(toRemove);
    return nextNode; // Pratique si tu itères sur la liste en même temps
}


// Dessine du texte blanc avec un fond gris arrondi semi-transparent
void render_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, OriginType origin) {
    SDL_Color color = {255, 255, 255, 255}; // Texte blanc
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture == NULL) {
        SDL_Log("Erreur lors de la création de la texture pour rendu texte : %s", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    int text_width = surface->w;
    int text_height = surface->h;

    // Ajuster les coordonnées en fonction de l’origine
    switch (origin) {
        case ORIGIN_UP_LEFT: break;
        case ORIGIN_UP_CENTER: x -= text_width / 2; break;
        case ORIGIN_UP_RIGHT: x -= text_width; break;
        case ORIGIN_MIDDLE_LEFT: y -= text_height / 2; break;
        case ORIGIN_MIDDLE_CENTER: x -= text_width / 2; y -= text_height / 2; break;
        case ORIGIN_MIDDLE_RIGHT: x -= text_width; y -= text_height / 2; break;
        case ORIGIN_DOWN_LEFT: y -= text_height; break;
        case ORIGIN_DOWN_CENTER: x -= text_width / 2; y -= text_height; break;
        case ORIGIN_DOWN_RIGHT: x -= text_width; y -= text_height; break;
    }

    SDL_Rect destRect = {x, y, text_width, text_height};

    // Calcul de la boîte de fond
    int padding = 4;
    SDL_Rect bgRect = {
        destRect.x - padding,
        destRect.y - padding,
        destRect.w + 2 * padding,
        destRect.h + 2 * padding
    };

    // Sauvegarder la couleur actuelle
    Uint8 r, g, b, a;
    SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);

    // Couleur de fond semi-transparente
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 120, 120, 120, 215);
    SDL_RenderFillRect(renderer, &bgRect);

    // Restaurer la couleur
    SDL_SetRenderDrawColor(renderer, r, g, b, a);

    // Afficher le texte
    SDL_RenderCopy(renderer, texture, NULL, &destRect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}


// Dessine une barre de chargement avec une progression donnée
void draw_loading_bar(SDL_Renderer* renderer, TTF_Font* font, char* text, int progress, int width, int height) {

    // Texte au dessus de la barre de chargement
    if (text != NULL) {
        render_text(renderer, font, text, width / 2, height / 2, ORIGIN_MIDDLE_CENTER);
    }
    
    // Coordonnées de base
    int barWidth = width * 0.4f;  // 40% de la largeur de la fenêtre
    int barHeight = 20;
    int barX = (width - barWidth) / 2;
    int barY = height / 2 + 25;

    // Bordure de la barre (fond)
    SDL_Rect backgroundRect = { barX, barY, barWidth, barHeight };
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);  // gris foncé
    SDL_RenderFillRect(renderer, &backgroundRect);

    // Calcul de la couleur entre rouge et vert
    int red = 255 * (100 - progress) / 100;
    int green = 255 * progress / 100;
    int blue = 0;

    // Barre de progression (remplissage)
    int filledWidth = (progress * barWidth) / 100;
    SDL_Rect filledRect = { barX, barY, filledWidth, barHeight };
    SDL_SetRenderDrawColor(renderer, red, green, blue, 255);
    SDL_RenderFillRect(renderer, &filledRect);

    // Optionnel : contour blanc
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &backgroundRect);
}


// Affiche un double de facon normale ou scientifique en fonction de sa taille
void format_float_auto(char* buffer, size_t size, double value) {
    if (fabs(value) < 0.001 || fabs(value) > 100000) {
        snprintf(buffer, size, "%.2e", value);  // notation scientifique
    } else {
        snprintf(buffer, size, "%.6f", value);  // notation décimale classique
    }
}


// Affichage dynamique en fonction du zoom
void format_float_zoom_adaptive(char* buffer, size_t buffer_size, double value, double zoom) {
    int decimals;

    // Idée simple : plus le zoom est grand, plus on affiche de décimales
    if (zoom <= 1)
        decimals = 2;
    else
        decimals = (int)(log10(zoom)) + 2;  // ex: zoom 100 → 4 décimales, zoom 1e6 → 8 décimales

    if (decimals > 15) decimals = 15;  // sécurité
    if (decimals < 2) decimals = 2;

    char formatString[32];
    snprintf(formatString, sizeof(formatString), "%%.%df", decimals);
    snprintf(buffer, buffer_size, formatString, value);
}


// transforme les coordonnée de l'écran en coordonnée de fractale
void screen_to_fractal(int x, int y, double zoom, double offsetX, double offsetY, int width, int height, double *fx, double *fy) {
    *fx = (x - width / 2) / zoom + offsetX;
    *fy = (y - height / 2) / zoom + offsetY;
}


// Clamp helper
double clamp_double(double val, double min, double max) {
    return (val < min) ? min : (val > max) ? max : val;
}


// Calcul de l'image du Mandelbrot en fonction des paramètres
int calculate_iterations(void* arg) {

    FractalTask* task = (FractalTask*)arg;

    *task->finished = false;
    *task->progress = 0;


    #ifdef __linux__
    // On appèlle la bonne fonction en fonction du type de fractale
    if (task->highPrecision) {
        switch (task->fractalType) {
            case FRACTAL_MANDELBROT:
                calculate_iterations_mandelbrot_mpfr(task);
                break;
            case FRACTAL_JULIA:
                calculate_iterations_julia_mpfr(task);
                break;
            case FRACTAL_BURNING_SHIP:
                calculate_iterations_burning_ship_mpfr(task);
                break;
            case FRACTAL_TRICORN:
                calculate_iterations_tricorn_mpfr(task);
                break;
            case FRACTAL_MULTIBROT:
                calculate_iterations_multibrot_mpfr(task);
                break;
        }
    } else {
        switch (task->fractalType) {
            case FRACTAL_MANDELBROT:
                calculate_iterations_mandelbrot(task);
                break;
            case FRACTAL_JULIA:
                calculate_iterations_julia(task);
                break;
            case FRACTAL_BURNING_SHIP:
                calculate_iterations_burning_ship(task);
                break;
            case FRACTAL_TRICORN:
                calculate_iterations_tricorn(task);
                break;
            case FRACTAL_MULTIBROT:
                calculate_iterations_multibrot(task);
                break;
        }
    }
    #else
    // On appèlle la bonne fonction en fonction du type de fractale
    switch (task->fractalType) {
        case FRACTAL_MANDELBROT:
            calculate_iterations_mandelbrot(task);
            break;
        case FRACTAL_JULIA:
            calculate_iterations_julia(task);
            break;
        case FRACTAL_BURNING_SHIP:
            calculate_iterations_burning_ship(task);
            break;
        case FRACTAL_TRICORN:
            calculate_iterations_tricorn(task);
            break;
        case FRACTAL_MULTIBROT:
            calculate_iterations_multibrot(task);
            break;
    }
    #endif

    // On inverse les deux listes seulement quand on est sur que l'écriture est complête
    int* iterationMapTemp = task->iterationMap;
    task->iterationMap = task->iterationMapBis;
    task->iterationMapBis = iterationMapTemp;

    *task->finished = true;
    return EXIT_SUCCESS;
}


// Calcul d'une image pour le type de Fractale: Mandelbrot
void calculate_iterations_mandelbrot(FractalTask* task) {
    int done = 0;
    for (int py = 0; py < task->height; py++) {
        for (int px = 0; px < task->width; px++) {
        
            double x0 = (px - task->width / 2.0) / task->zoom + task->offsetX;
            double y0 = (py - task->height / 2.0) / task->zoom + task->offsetY;
            double x = 0.0, y = 0.0;
            int iteration = 0;

            while (x * x + y * y <= 4.0 && iteration < task->max_iteration) {
                double xtemp = x * x - y * y + x0;
                y = 2.0 * x * y + y0;
                x = xtemp;
                iteration++;
            }

            task->iterationMapBis[py * task->width + px] = iteration;
            done++;
        }
        
        // On vérifie si on demande de sortir ou non
        if (task->shouldStop) {
            *task->progress = 100;
            *task->finished = true;
            return;
        }
        
        // Mettre à jour la progression une fois par ligne
        *task->progress = (done * 100) / (task->width * task->height);
    }
}


// Calcul d'une image pour le type de Fractale: Julia
void calculate_iterations_julia(FractalTask* task) {
    int done = 0;
    for (int py = 0; py < task->height; py++) {
        for (int px = 0; px < task->width; px++) {

            double x = (px - task->width / 2.0) / task->zoom + task->offsetX;
            double y = (py - task->height / 2.0) / task->zoom + task->offsetY;
            double cx = task->julia_c_re;
            double cy = task->julia_c_im;
            int iteration = 0;

            while (x * x + y * y <= 4.0 && iteration < task->max_iteration) {
                double xtemp = x * x - y * y + cx;
                y = 2.0 * x * y + cy;
                x = xtemp;
                iteration++;
            }

            task->iterationMapBis[py * task->width + px] = iteration;
            done++;
        }
        if (task->shouldStop) {
            *task->progress = 100;
            *task->finished = true;
            return;
        }
        *task->progress = (done * 100) / (task->width * task->height);
    }
}


// Calcul d'une image pour le type de Fractale: Burning Ship
void calculate_iterations_burning_ship(FractalTask* task) {
    int done = 0;
    for (int py = 0; py < task->height; py++) {
        for (int px = 0; px < task->width; px++) {

            double x0 = (px - task->width / 2.0) / task->zoom + task->offsetX;
            double y0 = (py - task->height / 2.0) / task->zoom + task->offsetY;
            double x = 0.0, y = 0.0;
            int iteration = 0;

            while (x * x + y * y <= 4.0 && iteration < task->max_iteration) {
                double xtemp = x * x - y * y + x0;
                y = fabs(2.0 * x * y) + y0;
                x = fabs(xtemp);
                iteration++;
            }

            task->iterationMapBis[py * task->width + px] = iteration;
            done++;
        }
        if (task->shouldStop) {
            *task->progress = 100;
            *task->finished = true;
            return;
        }
        *task->progress = (done * 100) / (task->width * task->height);
    }
}


// Calcul d'une image pour le type de Fractale: Tricorn
void calculate_iterations_tricorn(FractalTask* task) {
    int done = 0;
    for (int py = 0; py < task->height; py++) {
        for (int px = 0; px < task->width; px++) {

            double x0 = (px - task->width / 2.0) / task->zoom + task->offsetX;
            double y0 = (py - task->height / 2.0) / task->zoom + task->offsetY;
            double x = 0.0, y = 0.0;
            int iteration = 0;

            while (x * x + y * y <= 4.0 && iteration < task->max_iteration) {
                double xtemp = x * x - y * y + x0;
                y = -2.0 * x * y + y0;
                x = xtemp;
                iteration++;
            }

            task->iterationMapBis[py * task->width + px] = iteration;
            done++;
        }
        if (task->shouldStop) {
            *task->progress = 100;
            *task->finished = true;
            return;
        }
        *task->progress = (done * 100) / (task->width * task->height);
    }
}


// Calcul d'une image pour le type de Fractale: Multibrot
void calculate_iterations_multibrot(FractalTask* task) {
    int done = 0;
    int power = task->multibrot_power;
    for (int py = 0; py < task->height; py++) {
        for (int px = 0; px < task->width; px++) {

            double x0 = (px - task->width / 2.0) / task->zoom + task->offsetX;
            double y0 = (py - task->height / 2.0) / task->zoom + task->offsetY;
            double x = 0.0, y = 0.0;
            int iteration = 0;

            while (x * x + y * y <= 4.0 && iteration < task->max_iteration) {
                double r = hypot(x, y);
                double theta = atan2(y, x);
                double r_pow = pow(r, power);
                double new_x = r_pow * cos(power * theta) + x0;
                double new_y = r_pow * sin(power * theta) + y0;
                x = new_x;
                y = new_y;
                iteration++;
            }

            task->iterationMapBis[py * task->width + px] = iteration;
            done++;
        }
        if (task->shouldStop) {
            *task->progress = 100;
            *task->finished = true;
            return;
        }
        *task->progress = (done * 100) / (task->width * task->height);
    }
}


// Utilise une biliothèque permettant un zoom techniquement infini
#ifdef __linux__

void calculate_iterations_mandelbrot_mpfr(FractalTask* task) {
    int done = 0;
    mpfr_prec_t prec = 256; // Précision en bits, ajustez selon vos besoins

    mpfr_t x0, y0, x, y, xtemp, x_squared, y_squared, four;
    mpfr_inits2(prec, x0, y0, x, y, xtemp, x_squared, y_squared, four, (mpfr_ptr) 0);
    mpfr_set_d(four, 4.0, MPFR_RNDN);

    for (int py = 0; py < task->height; py++) {
        for (int px = 0; px < task->width; px++) {
            // x0 = (px - width / 2.0) / zoom + offsetX
            mpfr_set_d(x0, px - task->width / 2.0, MPFR_RNDN);
            mpfr_div_d(x0, x0, task->zoom, MPFR_RNDN);
            mpfr_add_d(x0, x0, task->offsetX, MPFR_RNDN);

            // y0 = (py - height / 2.0) / zoom + offsetY
            mpfr_set_d(y0, py - task->height / 2.0, MPFR_RNDN);
            mpfr_div_d(y0, y0, task->zoom, MPFR_RNDN);
            mpfr_add_d(y0, y0, task->offsetY, MPFR_RNDN);

            mpfr_set_d(x, 0.0, MPFR_RNDN);
            mpfr_set_d(y, 0.0, MPFR_RNDN);
            int iteration = 0;

            while (iteration < task->max_iteration) {
                // x_squared = x * x
                mpfr_mul(x_squared, x, x, MPFR_RNDN);
                // y_squared = y * y
                mpfr_mul(y_squared, y, y, MPFR_RNDN);

                // x_squared + y_squared > 4.0 ?
                mpfr_add(xtemp, x_squared, y_squared, MPFR_RNDN);
                if (mpfr_cmp(xtemp, four) > 0)
                    break;

                // xtemp = x_squared - y_squared + x0
                mpfr_sub(xtemp, x_squared, y_squared, MPFR_RNDN);
                mpfr_add(xtemp, xtemp, x0, MPFR_RNDN);

                // y = 2 * x * y + y0
                mpfr_mul(y, x, y, MPFR_RNDN);
                mpfr_mul_d(y, y, 2.0, MPFR_RNDN);
                mpfr_add(y, y, y0, MPFR_RNDN);

                // x = xtemp
                mpfr_set(x, xtemp, MPFR_RNDN);

                iteration++;
            }

            task->iterationMapBis[py * task->width + px] = iteration;
            done++;
        }

        if (task->shouldStop) {
            *task->progress = 100;
            *task->finished = true;
            mpfr_clears(x0, y0, x, y, xtemp, x_squared, y_squared, four, (mpfr_ptr) 0);
            return;
        }

        *task->progress = (done * 100) / (task->width * task->height);
    }

    mpfr_clears(x0, y0, x, y, xtemp, x_squared, y_squared, four, (mpfr_ptr) 0);
}


void calculate_iterations_julia_mpfr(FractalTask* task) {
    int done = 0;
    mpfr_prec_t prec = 256;

    mpfr_t x, y, xtemp, x_squared, y_squared, two_xy, cx, cy, four;
    mpfr_inits2(prec, x, y, xtemp, x_squared, y_squared, two_xy, cx, cy, four, (mpfr_ptr) 0);
    mpfr_set_d(four, 4.0, MPFR_RNDN);
    mpfr_set_d(cx, task->julia_c_re, MPFR_RNDN);
    mpfr_set_d(cy, task->julia_c_im, MPFR_RNDN);

    for (int py = 0; py < task->height; py++) {
        for (int px = 0; px < task->width; px++) {
            // Coordonnées initiales
            mpfr_set_d(x, px - task->width / 2.0, MPFR_RNDN);
            mpfr_div_d(x, x, task->zoom, MPFR_RNDN);
            mpfr_add_d(x, x, task->offsetX, MPFR_RNDN);

            mpfr_set_d(y, py - task->height / 2.0, MPFR_RNDN);
            mpfr_div_d(y, y, task->zoom, MPFR_RNDN);
            mpfr_add_d(y, y, task->offsetY, MPFR_RNDN);

            int iteration = 0;
            while (iteration < task->max_iteration) {
                mpfr_mul(x_squared, x, x, MPFR_RNDN);
                mpfr_mul(y_squared, y, y, MPFR_RNDN);
                mpfr_add(xtemp, x_squared, y_squared, MPFR_RNDN);
                if (mpfr_cmp(xtemp, four) > 0)
                    break;

                // xtemp = x² - y² + cx
                mpfr_sub(xtemp, x_squared, y_squared, MPFR_RNDN);
                mpfr_add(xtemp, xtemp, cx, MPFR_RNDN);

                // y = 2xy + cy
                mpfr_mul(two_xy, x, y, MPFR_RNDN);
                mpfr_mul_d(two_xy, two_xy, 2.0, MPFR_RNDN);
                mpfr_add(y, two_xy, cy, MPFR_RNDN);

                // x = xtemp
                mpfr_set(x, xtemp, MPFR_RNDN);

                iteration++;
            }

            task->iterationMapBis[py * task->width + px] = iteration;
            done++;
        }

        if (task->shouldStop) {
            *task->progress = 100;
            *task->finished = true;
            mpfr_clears(x, y, xtemp, x_squared, y_squared, two_xy, cx, cy, four, (mpfr_ptr) 0);
            return;
        }

        *task->progress = (done * 100) / (task->width * task->height);
    }

    mpfr_clears(x, y, xtemp, x_squared, y_squared, two_xy, cx, cy, four, (mpfr_ptr) 0);
}


void calculate_iterations_burning_ship_mpfr(FractalTask* task) {
    int done = 0;
    mpfr_prec_t prec = 256;

    mpfr_t x0, y0, x, y, xtemp, x_squared, y_squared, two_xy, four;
    mpfr_inits2(prec, x0, y0, x, y, xtemp, x_squared, y_squared, two_xy, four, (mpfr_ptr) 0);
    mpfr_set_d(four, 4.0, MPFR_RNDN);

    for (int py = 0; py < task->height; py++) {
        for (int px = 0; px < task->width; px++) {
            // x0
            mpfr_set_d(x0, px - task->width / 2.0, MPFR_RNDN);
            mpfr_div_d(x0, x0, task->zoom, MPFR_RNDN);
            mpfr_add_d(x0, x0, task->offsetX, MPFR_RNDN);

            // y0
            mpfr_set_d(y0, py - task->height / 2.0, MPFR_RNDN);
            mpfr_div_d(y0, y0, task->zoom, MPFR_RNDN);
            mpfr_add_d(y0, y0, task->offsetY, MPFR_RNDN);

            mpfr_set_d(x, 0.0, MPFR_RNDN);
            mpfr_set_d(y, 0.0, MPFR_RNDN);
            int iteration = 0;

            while (iteration < task->max_iteration) {
                // x², y²
                mpfr_mul(x_squared, x, x, MPFR_RNDN);
                mpfr_mul(y_squared, y, y, MPFR_RNDN);
                mpfr_add(xtemp, x_squared, y_squared, MPFR_RNDN);
                if (mpfr_cmp(xtemp, four) > 0)
                    break;

                // xtemp = |x² - y² + x0|
                mpfr_sub(xtemp, x_squared, y_squared, MPFR_RNDN);
                mpfr_add(xtemp, xtemp, x0, MPFR_RNDN);
                mpfr_abs(x, xtemp, MPFR_RNDN);

                // y = |2xy + y0|
                mpfr_mul(two_xy, x, y, MPFR_RNDN);
                mpfr_mul_d(two_xy, two_xy, 2.0, MPFR_RNDN);
                mpfr_add(y, two_xy, y0, MPFR_RNDN);
                mpfr_abs(y, y, MPFR_RNDN);

                iteration++;
            }

            task->iterationMapBis[py * task->width + px] = iteration;
            done++;
        }

        if (task->shouldStop) {
            *task->progress = 100;
            *task->finished = true;
            mpfr_clears(x0, y0, x, y, xtemp, x_squared, y_squared, two_xy, four, (mpfr_ptr) 0);
            return;
        }

        *task->progress = (done * 100) / (task->width * task->height);
    }

    mpfr_clears(x0, y0, x, y, xtemp, x_squared, y_squared, two_xy, four, (mpfr_ptr) 0);
}



void calculate_iterations_tricorn_mpfr(FractalTask* task) {
    int done = 0;
    mpfr_prec_t prec = 256;

    mpfr_t x0, y0, x, y, xtemp, x_squared, y_squared, two_xy, four;
    mpfr_inits2(prec, x0, y0, x, y, xtemp, x_squared, y_squared, two_xy, four, (mpfr_ptr) 0);
    mpfr_set_d(four, 4.0, MPFR_RNDN);

    for (int py = 0; py < task->height; py++) {
        for (int px = 0; px < task->width; px++) {
            // Position dans le plan complexe
            mpfr_set_d(x0, px - task->width / 2.0, MPFR_RNDN);
            mpfr_div_d(x0, x0, task->zoom, MPFR_RNDN);
            mpfr_add_d(x0, x0, task->offsetX, MPFR_RNDN);

            mpfr_set_d(y0, py - task->height / 2.0, MPFR_RNDN);
            mpfr_div_d(y0, y0, task->zoom, MPFR_RNDN);
            mpfr_add_d(y0, y0, task->offsetY, MPFR_RNDN);

            mpfr_set_d(x, 0.0, MPFR_RNDN);
            mpfr_set_d(y, 0.0, MPFR_RNDN);
            int iteration = 0;

            while (iteration < task->max_iteration) {
                mpfr_mul(x_squared, x, x, MPFR_RNDN);
                mpfr_mul(y_squared, y, y, MPFR_RNDN);
                mpfr_add(xtemp, x_squared, y_squared, MPFR_RNDN);
                if (mpfr_cmp(xtemp, four) > 0)
                    break;

                // xtemp = x² - y² + x0
                mpfr_sub(xtemp, x_squared, y_squared, MPFR_RNDN);
                mpfr_add(xtemp, xtemp, x0, MPFR_RNDN);

                // y = -2xy + y0
                mpfr_mul(two_xy, x, y, MPFR_RNDN);
                mpfr_mul_d(two_xy, two_xy, -2.0, MPFR_RNDN);
                mpfr_add(y, two_xy, y0, MPFR_RNDN);

                mpfr_set(x, xtemp, MPFR_RNDN);
                iteration++;
            }

            task->iterationMapBis[py * task->width + px] = iteration;
            done++;
        }

        if (task->shouldStop) {
            *task->progress = 100;
            *task->finished = true;
            mpfr_clears(x0, y0, x, y, xtemp, x_squared, y_squared, two_xy, four, (mpfr_ptr) 0);
            return;
        }

        *task->progress = (done * 100) / (task->width * task->height);
    }

    mpfr_clears(x0, y0, x, y, xtemp, x_squared, y_squared, two_xy, four, (mpfr_ptr) 0);
}


void calculate_iterations_multibrot_mpfr(FractalTask* task) {
    int done = 0;
    int power = task->multibrot_power;
    mpfr_prec_t prec = 256;

    mpfr_t x0, y0, x, y, r, theta, r_pow, angle, new_x, new_y, x_squared, y_squared, sum, four;
    mpfr_inits2(prec, x0, y0, x, y, r, theta, r_pow, angle, new_x, new_y, x_squared, y_squared, sum, four, (mpfr_ptr) 0);
    mpfr_set_d(four, 4.0, MPFR_RNDN);

    for (int py = 0; py < task->height; py++) {
        for (int px = 0; px < task->width; px++) {
            // Coordonnées initiales
            mpfr_set_d(x0, px - task->width / 2.0, MPFR_RNDN);
            mpfr_div_d(x0, x0, task->zoom, MPFR_RNDN);
            mpfr_add_d(x0, x0, task->offsetX, MPFR_RNDN);

            mpfr_set_d(y0, py - task->height / 2.0, MPFR_RNDN);
            mpfr_div_d(y0, y0, task->zoom, MPFR_RNDN);
            mpfr_add_d(y0, y0, task->offsetY, MPFR_RNDN);

            mpfr_set_d(x, 0.0, MPFR_RNDN);
            mpfr_set_d(y, 0.0, MPFR_RNDN);
            int iteration = 0;

            while (iteration < task->max_iteration) {
                // r = sqrt(x^2 + y^2)
                mpfr_mul(x_squared, x, x, MPFR_RNDN);
                mpfr_mul(y_squared, y, y, MPFR_RNDN);
                mpfr_add(sum, x_squared, y_squared, MPFR_RNDN);
                if (mpfr_cmp(sum, four) > 0)
                    break;
                mpfr_sqrt(r, sum, MPFR_RNDN);

                // theta = atan2(y, x)
                mpfr_atan2(theta, y, x, MPFR_RNDN);

                // r^power
                mpfr_pow_ui(r_pow, r, power, MPFR_RNDN);

                // angle = power * theta
                mpfr_mul_si(angle, theta, power, MPFR_RNDN);

                // new_x = r^power * cos(angle) + x0
                mpfr_cos(new_x, angle, MPFR_RNDN);
                mpfr_mul(new_x, r_pow, new_x, MPFR_RNDN);
                mpfr_add(new_x, new_x, x0, MPFR_RNDN);

                // new_y = r^power * sin(angle) + y0
                mpfr_sin(new_y, angle, MPFR_RNDN);
                mpfr_mul(new_y, r_pow, new_y, MPFR_RNDN);
                mpfr_add(new_y, new_y, y0, MPFR_RNDN);

                mpfr_set(x, new_x, MPFR_RNDN);
                mpfr_set(y, new_y, MPFR_RNDN);
                iteration++;
            }

            task->iterationMapBis[py * task->width + px] = iteration;
            done++;
        }

        if (task->shouldStop) {
            *task->progress = 100;
            *task->finished = true;
            mpfr_clears(x0, y0, x, y, r, theta, r_pow, angle, new_x, new_y, x_squared, y_squared, sum, four, (mpfr_ptr) 0);
            return;
        }

        *task->progress = (done * 100) / (task->width * task->height);
    }

    mpfr_clears(x0, y0, x, y, r, theta, r_pow, angle, new_x, new_y, x_squared, y_squared, sum, four, (mpfr_ptr) 0);
}

#endif


// Fait le rendu en couleurs des itérations sur la cible SDL
void render_iterations(SDL_Renderer *renderer, int *iterationMap, int w, int h, SDL_Color *palette, int max_iteration, bool antialiasing) {

    for (int py = 0; py < h; py++) {
        for (int px = 0; px < w; px++) {
            int iteration;

            if (antialiasing) {
                // Antialiasing activé : moyenne avec les pixels voisins
                int neighboringIterations = iterationMap[py * w + px];
                int count = 1;

                if (px > 0) {
                    neighboringIterations += iterationMap[py * w + (px - 1)];
                    count++;
                }
                if (px < w - 1) {
                    neighboringIterations += iterationMap[py * w + (px + 1)];
                    count++;
                }
                if (py > 0) {
                    neighboringIterations += iterationMap[(py - 1) * w + px];
                    count++;
                }
                if (py < h - 1) {
                    neighboringIterations += iterationMap[(py + 1) * w + px];
                    count++;
                }

                iteration = neighboringIterations / count;
            } else {
                // Antialiasing désactivé : on utilise la valeur brute
                iteration = iterationMap[py * w + px];
            }

            // On va sélectionner la couleur de chaque pixel depuis la palette pré-générée
            if (iteration >= max_iteration) {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            } else {
                int colorIndex = (iteration * (PALETTE_SIZE - 1)) / max_iteration;

                // Clamp pour éviter les segfaults
                if (colorIndex < 0) colorIndex = 0;
                if (colorIndex >= PALETTE_SIZE) colorIndex = PALETTE_SIZE - 1;

                SDL_Color color = palette[colorIndex];
                SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
            }
            
            SDL_RenderDrawPoint(renderer, px, py);
        }
    }
}


// Appelle toute les fonctions nécéssaire a l'affichage de la texture proportionnel au zoom et au coordonnées
int draw_all_textures(SDL_Renderer *renderer, int windowWidth, int windowHeight, double zoom, double offsetX, double offsetY, 
                      FractalList* texturesList, FractalList* selectedTexture, bool selectTextureOn) {

    bool errorValue = false;

    // Parcours la liste chainée de textures
    FractalList* actualTexture = texturesList;
    while (actualTexture != NULL) {
            
        // Vérifie si la texture est valide, et l'ignore sinon
        if (SDL_QueryTexture(actualTexture->texture, NULL, NULL, NULL, NULL) != 0) {
            errorValue = true;
            continue;
        }
            
        SDL_Color frameRectColor;
        bool drawArrow, drawFrameRect;

        // Si on n'est pas en mode sélection de texture, n'afficher ni la flèche ni les contours
        if (!selectTextureOn) {
            drawFrameRect = false;
            drawArrow = false;
        // Si on est en mode sélection de textures et qu'on est sur l'image sélectionnée, afficher flèches et contours verts
        } else if (actualTexture == selectedTexture) {
            drawFrameRect = true;
            frameRectColor = (SDL_Color){0, 255, 0, 255};
            drawArrow = true;
        // Si on est en mode sélection de textures et que l'on n'est pas sur l'image sélectionnée, afficher contours blanc
        } else if (actualTexture != selectedTexture) {
            drawFrameRect = true;
            frameRectColor = (SDL_Color){255, 255, 255, 255};
            drawArrow = false;
        }
        
        // Le ratio entre le zoom précédent et actuel
        double zoomRatio = zoom / actualTexture->zoom;

        // Calcule les coins dans l'ancien espace fractal (avant zoom)
        double fx1, fy1, fx2, fy2;
        screen_to_fractal(0, 0, zoom, offsetX, offsetY, windowWidth, windowHeight, &fx1, &fy1);
        screen_to_fractal(windowWidth, windowHeight, zoom, offsetX, offsetY, windowWidth, windowHeight, &fx2, &fy2);
        
        
        // Reprojette les coins (fx1, fy1) et (fx2, fy2) en pixels dans l’ancienne texture
        double sx1 = (fx1 - actualTexture->offsetX) * actualTexture->zoom + actualTexture->width / 2.0;
        double sy1 = (fy1 - actualTexture->offsetY) * actualTexture->zoom + actualTexture->height / 2.0;
        double sx2 = (fx2 - actualTexture->offsetX) * actualTexture->zoom + actualTexture->width / 2.0;
        double sy2 = (fy2 - actualTexture->offsetY) * actualTexture->zoom + actualTexture->height / 2.0;


        // On limite les valeurs des positions maximales
        sx1 = clamp_double(sx1, 0.0, actualTexture->width  - 1.0);
        sy1 = clamp_double(sy1, 0.0, actualTexture->height - 1.0);
        sx2 = clamp_double(sx2, 0.0, actualTexture->width  - 1.0);
        sy2 = clamp_double(sy2, 0.0, actualTexture->height - 1.0);

        // Calculer le rectangle source à partir de la texture
        SDL_Rect srcRect = (SDL_Rect){
            .x = smallest((int)sx1, (int)sx2),
            .y = smallest((int)sy1, (int)sy2),
            .w = abs((int)sx2 - (int)sx1) + 1,
            .h = abs((int)sy2 - (int)sy1) + 1
        };



        // Rectangle de destination
        SDL_Rect destRect;

        // Destination : fenêtre de rendu, ajustée au nouveau zoom
        destRect.w = (int)(actualTexture->width * zoomRatio);
        destRect.h = (int)(actualTexture->height * zoomRatio);

        destRect.x = (windowWidth - destRect.w) / 2 + (int)((actualTexture->offsetX - offsetX) * zoom);
        destRect.y = (windowHeight - destRect.h) / 2 + (int)((actualTexture->offsetY - offsetY) * zoom);
        


        // Ne compense pas si on est déjà au bord (sinon bugs visuels)
        // Détecte si on touche le bord droit ou bas de la texture
        
        double offsetIntDoubleDestW;
        double offsetIntDoubleDestH;

        if (destRect.x + destRect.w > windowWidth) {
            offsetIntDoubleDestW = (srcRect.w - fabs(sx2 - sx1));
        } else {
            offsetIntDoubleDestW = -(srcRect.w - fabs(sx2 - sx1));
        }

        if (destRect.y + destRect.h > windowHeight) {
            offsetIntDoubleDestH = (srcRect.h - fabs(sy2 - sy1));
        } else {
            offsetIntDoubleDestH = -(srcRect.h - fabs(sy2 - sy1));
        }
        
        // Clamp le destRect à l’intérieur de la fenêtre
        if (destRect.x < 0) {
            destRect.w += destRect.x - offsetIntDoubleDestW * zoomRatio;
            destRect.x = 0;
        }
        if (destRect.y < 0) {
            destRect.h += destRect.y - offsetIntDoubleDestH * zoomRatio;
            destRect.y = 0;
        }
        if (destRect.x + destRect.w > windowWidth) {
            destRect.w = windowWidth - destRect.x + offsetIntDoubleDestW * zoomRatio;
        }
        if (destRect.y + destRect.h > windowHeight) {
            destRect.h = windowHeight - destRect.y + offsetIntDoubleDestH * zoomRatio;
        }
        

        // Calcule le sous-pixel offset à partir de la vraie position flottante
        double fractalShiftX = (sx1 < sx2 ? sx1 : sx2) - (double)srcRect.x;
        double fractalShiftY = (sy1 < sy2 ? sy1 : sy2) - (double)srcRect.y;

        double subPixelOffsetX = fractalShiftX * zoomRatio;
        double subPixelOffsetY = fractalShiftY * zoomRatio;

        // Ajuste la destination
        destRect.x -= (int)round(subPixelOffsetX);
        destRect.y -= (int)round(subPixelOffsetY);

        // Ajuste la source (pour rester aligné avec le zoom)
        srcRect.x += (int)floor(subPixelOffsetX / zoomRatio);
        srcRect.y += (int)floor(subPixelOffsetY / zoomRatio);
        
        // Si la texture à dessiner est en dehors de l'écran
        bool isOutsideScreen = (destRect.x + destRect.w < 0 || destRect.x > windowWidth || destRect.y + destRect.h < 0 || destRect.y > windowHeight);
        
        // Affichage de la flèche qui pointe vers la texture si en dehors de l'écran
        if (isOutsideScreen && drawArrow) {
            int centerTexX = destRect.x + destRect.w / 2;
            int centerTexY = destRect.y + destRect.h / 2;

            int centerWinX = windowWidth / 2;
            int centerWinY = windowHeight / 2;

            double dx = centerTexX - centerWinX;
            double dy = centerTexY - centerWinY;

            double angle = atan2(dy, dx);

            double borderX = cos(angle);
            double borderY = sin(angle);

            double scale = fmin(
                fabs((windowWidth / 2.0 - 10) / borderX),
                fabs((windowHeight / 2.0 - 10) / borderY)
            );

            int arrowX = (int)(centerWinX + borderX * scale);
            int arrowY = (int)(centerWinY + borderY * scale);

            double size = 30;
            double leftAngle = angle + M_PI * 0.75;
            double rightAngle = angle - M_PI * 0.75;

            // Points du triangle
            SDL_Vertex verts[3];
            SDL_Color arrowColor = {255, 0, 0, 255}; // tu peux modifier cette variable ailleurs

            verts[0].position.x = (float)arrowX;
            verts[0].position.y = (float)arrowY;
            verts[0].color = arrowColor;

            verts[1].position.x = (float)(arrowX + cos(leftAngle) * size);
            verts[1].position.y = (float)(arrowY + sin(leftAngle) * size);
            verts[1].color = arrowColor;

            verts[2].position.x = (float)(arrowX + cos(rightAngle) * size);
            verts[2].position.y = (float)(arrowY + sin(rightAngle) * size);
            verts[2].color = arrowColor;

            // Dessine un triangle plein
            SDL_RenderGeometry(renderer, NULL, verts, 3, NULL, 0);
        }
        
        if (!isOutsideScreen) {
        
            // Imprime la texture bien placée sur la cible sélectionnée avant la fonction
            SDL_RenderCopy(renderer, actualTexture->texture, &srcRect, &destRect);
            
            // Si on dessine le frame autour de la texture
            if (drawFrameRect) {
            
                // Pour le dessin du rectangle autour de la texture
                SDL_Rect frameRect = {
                    .x = destRect.x - 2,
                    .y = destRect.y - 2,
                    .w = destRect.w + 4,
                    .h = destRect.h + 4
                };
                
                // Dessiner un bord autour de la texture
                SDL_SetRenderDrawColor(renderer, frameRectColor.r, frameRectColor.g, frameRectColor.b, frameRectColor.a);
                SDL_RenderDrawRect(renderer, &frameRect);
            }
        }
        
        actualTexture = actualTexture->next;
    }
    
    return errorValue;
}




