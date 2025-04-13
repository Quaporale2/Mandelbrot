
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

// Pour l'éxécution d'un thread secondaire
//#include <pthread.h>
//#include <stdatomic.h>

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


// Pour la séparation en un deuxième thread lors du calcul
typedef struct {
    int *iterationMap;
    int max_iteration;
    int actual_max;
    double zoom, offsetX, offsetY;
    int width, height;
    bool antialiasing;

    int progress;  // De 0 à 100
    bool finished;
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
    WHITE_BLACK = 2
} ColorSchemes;

// Liste de l'historique des zooms
FractalView history[MAX_HISTORY];
int historyIndex = -1;

// La palette de couleur que va utiliser le Mandelbrot
SDL_Color palette[PALETTE_SIZE];



// Gestion des palette de couleurs du Mandelbrot
void generate_palette_hot_cold();
void generate_palette_white_black();

// Gestion de l'historique de position de l'image
void push_view(double zoom, double offsetX, double offsetY);
bool pop_view(double *zoom, double *offsetX, double *offsetY);

// Calcul de l'image du Mandelbrot
//void draw_mandelbrot_normal(SDL_Renderer *renderer, SDL_Texture *texture, int max_iteration, double zoom, double offsetX, double offsetY, int width, int height, bool antialiasing);
int calculate_iterations(void* arg);
void render_iterations(SDL_Renderer *renderer, int *iterationMap, int w, int h, SDL_Color *palette, int max_iteration, int actual_max, bool antialiasing);
#ifdef __linux__
    void draw_mandelbrot_high_precision(SDL_Renderer *renderer, SDL_Texture *texture, int max_iteration, double zoom, double offsetX, double offsetY, int width, int height, bool antialiasing);
#endif


// Calcul des positions sur l'écran par rapport au positions dans la fractale
void screen_to_fractal(int x, int y, double zoom, double offsetX, double offsetY, int width, int height, double *fx, double *fy);

// Calcul du rectangle source et du rectangle de destination, pour le placement de la texture actuelle en fonction de la position précédente et actuelle
void calculate_rectangles(double fx1, double fy1, double fx2, double fy2, int windowWidth, int windowHeight, double zoom, double lastZoom, double lastOffsetX, double lastOffsetY, double offsetX, double offsetY, SDL_Rect *srcRect, SDL_Rect *destRect);

// Dessine la texture du Mandelbrot en prenant une partie d'une texture, et la collant sur une partie d'une autre texture
void draw_mandelbrot_well_placed(SDL_Renderer *renderer, SDL_Texture *texture, int windowWidth, int windowHeight, double zoom, double lastZoom, double lastOffsetX, double lastOffsetY, double offsetX, double offsetY);



// Dessine le texte passé en paramètre
void render_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, OriginType origin);





int main(int argc, char *argv[]) {


     /* * * * * * * * * */
    /* Choix utilisateur */
    
    // Indique si on cache le texte ou non
    bool hideInterface = false;
    
    // Active ou non l'antialiasing du Mandelbrot
    bool activateAntialiasing = true;
    
    // Définit si le calcul du mandelbrot sera précis ou normal
    // Seulement dans la version linux
    #ifdef __linux__
        bool advancedMode = false;
    #endif
    
    // Si activé, la mise à jour auto du mandelbrot au modification de zoom et d'offset ne se fonts plus
    bool activateAutoRefresh = false;
    
    // Donne le nombre d'itérations jusqu'a lequel on va a chaque calcul de pixel du mandelbrot
    int max_iteration = 200;

    // Valeur de zoom et d'offset par défaut (position de départ)
    double zoom = 200.0;
    double offsetX = -0.5;
    double offsetY = 0.0;
    
    // Définit les couleurs utilisées
    int colorScheme = HOT_COLD;



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
    const char fontPath[] = "DejaVuSans.ttf";

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
    bool queryCalculateImage = false;
    bool calculateImage = true;
    bool drawingMade = false;
    
    
    
    // Après modification de taille de la fenêtre, indique si on attend le clic de déblocage
    bool initialClickDone = true;
    
    // Indique le menu actuel ouvert (1: sélection précision normale/précise. 2: Entrée du nombre maximal d'itérations)
    uint8_t menuMode = 0;
    
    // Buffer de texte d'entrée et de sortie
    char inputBuffer[100];
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
    
    bool fractalCalcPending = false;
    
    // Initialise la police d'écriture
    TTF_Init();
    TTF_Font *font = TTF_OpenFont(fontPath, (int)(8 + windowWidth * 0.006));
    if (!font) {
        SDL_Log("Erreur chargement police d'écriture : %s", TTF_GetError());
        return 1;
    }

    // Initialise le moteur graphique et la fenêtre
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow(windowName, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, SDL_WINDOW_RESIZABLE);
    
    // Taille minimale de la fenêtre
    SDL_SetWindowMinimumSize(window, minWindowWidth, minWindowHeight);
    
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Ce qui va contenir tout la texture de la fractale
    SDL_Texture *fractalTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, windowWidth, windowHeight);

    // Contient les évenement de la fenêtre
    SDL_Event event;
    
    FractalTask task;
    task.iterationMap = malloc(windowWidth * windowHeight * sizeof(int));
    task.max_iteration = 0;
    task.actual_max = 0;
    task.zoom = 0;
    task.offsetX = 0;
    task.offsetY = 0;
    task.width = 0;
    task.height = 0;
    task.antialiasing = false;
    task.progress = 0;
    task.finished = false;


    // Génère la palette de couleurs qui va servir à colorer le mandelbrot
    switch (colorScheme) {
        case HOT_COLD:
            generate_palette_hot_cold();
            break;
        case WHITE_BLACK:
            generate_palette_white_black();
            break;
    }


    // Boucle principale d'éxécution
    while (running) {
    
        // On y passe tant qu'on a des évenements à traiter
        while (SDL_PollEvent(&event)) {
        
            // Si on ferme la page
            if (event.type == SDL_QUIT)
                running = false;
                
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
                font = TTF_OpenFont(fontPath, (int)(8 + windowWidth * 0.006));
                if (!font) {
                    SDL_Log("Erreur chargement police d'écriture : %s", TTF_GetError());
                    return 1;
                }
                
                // Ferme tout les menus actifs
                menuMode = no_menu;
                
                initialClickDone = false;
            }
            // Revient en arrière dans l'historique sur clic gauche
            if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_RIGHT && !rightDragging && !menuMode && initialClickDone) {
                if (pop_view(&zoom, &offsetX, &offsetY)) {
                    queryCalculateImage = true;
                }
            }
            // Clic molette ou espace recalcule le mandelbrot
            if (((event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_MIDDLE) || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE)) && !menuMode && initialClickDone) {
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
                    push_view(zoom, offsetX, offsetY);
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
                queryCalculateImage = true;
            }
            if (event.type == SDL_KEYDOWN && initialClickDone && !menuMode) {
                switch (event.key.keysym.sym) {
                    // Flêche haut
                    case SDLK_UP:
                        // Si on vient de changer d'action de mouvement, enregistrer la position dans l'historique
                        if (event.type != lastActionType || event.key.keysym.sym != lastActionValue) {
                            push_view(zoom, offsetX, offsetY);
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
                            push_view(zoom, offsetX, offsetY);
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
                            push_view(zoom, offsetX, offsetY);
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
                            push_view(zoom, offsetX, offsetY);
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
                            push_view(zoom, offsetX, offsetY);
                            lastActionType = SDL_KEYDOWN;
                            lastActionValue = event.key.keysym.sym;
                        }
                        zoom *= 1.6;
                        redrawInterface = true;
                        queryCalculateImage = true;
                        break;
                    // Touche moins
                    case SDLK_MINUS:
                        // Si on vient de changer d'action de mouvement, enregistrer la position dans l'historique
                        if (event.type != lastActionType || event.key.keysym.sym != lastActionValue) {
                            push_view(zoom, offsetX, offsetY);
                            lastActionType = SDL_KEYDOWN;
                            lastActionValue = event.key.keysym.sym;
                        }
                        zoom /= 1.6;
                        redrawInterface = true;
                        queryCalculateImage = true;
                        break;
                }
                // Toggle pour cacher/montrer l'affichage avec la touche H
                if (event.key.keysym.sym == SDLK_h && !menuMode && initialClickDone) {
                    hideInterface = !hideInterface;
                    redrawInterface = true;
                }
                // Toggle pour activer/désactiver l'antialiasing avec la touche J
                if (event.key.keysym.sym == SDLK_j && !menuMode && initialClickDone) {
                    activateAntialiasing = !activateAntialiasing;
                    redrawInterface = true;
                    queryCalculateImage = true;
                }
                // Toggle pour activer/désactiver l'autorefresh de l'image du mandelbrot avec la touche R
                if (event.key.keysym.sym == SDLK_r && !menuMode && initialClickDone) {
                    activateAutoRefresh = !activateAutoRefresh;
                    redrawInterface = true;
                }
                // Toggle pour activer/désactiver l'autorefresh le mode de précision élevé avec la touche M
                // Précision complexe seulement dans la version linux
                #ifdef __linux__
                    if (event.key.keysym.sym == SDLK_m && !menuMode && initialClickDone) {
                        advancedMode = !advancedMode;
                        redrawInterface = true;
                        queryCalculateImage = true;
                    }
                #endif
                // Passe à travers les différentes Palettes de couleurs avec la touche B
                if (event.key.keysym.sym == SDLK_b && !menuMode && initialClickDone) {
                    switch (colorScheme) {
                        case HOT_COLD:
                            colorScheme = WHITE_BLACK;
                            generate_palette_white_black();
                            redrawInterface = true;
                            queryCalculateImage = true;
                            break;
                        case WHITE_BLACK:
                            colorScheme = HOT_COLD;
                            generate_palette_hot_cold();
                            redrawInterface = true;
                            queryCalculateImage = true;
                            break;
                    }
                }
                // Menu entrée du nombre max d'itérations
                if (event.key.keysym.sym == SDLK_i && !menuMode && initialClickDone) {
                    menuMode = max_iteration_menu;
                    rightSelecting = false;
                    leftSelecting = false;

                    
                    SDL_RenderCopy(renderer, fractalTexture, NULL, NULL);
                    render_text(renderer, font, "Veuillez entrer le nouveau nombre maximal d'itérations: ", 10, 10, ORIGIN_UP_LEFT);
                    SDL_RenderPresent(renderer);
                    
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
                        push_view(zoom, offsetX, offsetY);
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
                
                push_view(zoom, offsetX, offsetY);

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
                queryCalculateImage = true;
            }
            
            // Gestion de l'actualisation de la taille de la texture au changement de taille d'écran
            if (event.type == SDL_MOUSEBUTTONDOWN && (event.button.button == SDL_BUTTON_LEFT || event.button.button == SDL_BUTTON_RIGHT) && !menuMode && !initialClickDone) {
                
                // 1. Sauvegarder l’ancienne texture et sa taille
                SDL_Texture* oldTexture = fractalTexture;
                int oldW, oldH;
                SDL_QueryTexture(oldTexture, NULL, NULL, &oldW, &oldH);

                // 2. Créer la nouvelle texture avec la nouvelle taille
                fractalTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, windowWidth, windowHeight);

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
                SDL_SetRenderTarget(renderer, NULL);
                SDL_DestroyTexture(oldTexture);
                
                // Actualiser la taille de la map d'itérations
                free(task.iterationMap);
                task.iterationMap = malloc(windowWidth * windowHeight * sizeof(int));

                initialClickDone = true;
                redrawInterface = true;
                queryCalculateImage = true;
            }

            // Si le menu d'entrée du nombre max d'itérations est activé, on surveille les inputs liés
            if (menuMode == max_iteration_menu) {
            
                bool stringModified = false;
                
                if (event.type == SDL_TEXTINPUT) {
                    if (strlen(inputBuffer) + strlen(event.text.text) < sizeof(inputBuffer) - 1) {
                        // N'ajoute que les chiffres
                        for (int i = 0; event.text.text[i]; i++) {
                            if (isdigit(event.text.text[i])) {
                                int len = strlen(inputBuffer);
                                inputBuffer[len] = event.text.text[i];
                                inputBuffer[len + 1] = '\0';
                            }
                        }
                    }
                    stringModified = true;

                } else if (event.type == SDL_KEYDOWN) {
                    if (event.key.keysym.sym == SDLK_RETURN && strlen(inputBuffer) > 0) {
                        max_iteration = atoi(inputBuffer);
                        menuMode = no_menu;
                        SDL_StopTextInput();
                        queryCalculateImage = true;
                    } else if (event.key.keysym.sym == SDLK_BACKSPACE && strlen(inputBuffer) > 0) {
                        inputBuffer[strlen(inputBuffer) - 1] = '\0';
                        stringModified = true;
                    } else if (event.key.keysym.sym == SDLK_ESCAPE) {
                        menuMode = no_menu;
                        SDL_StopTextInput();
                        redrawInterface = true;
                    }
                } else {
                    redrawInterface = false;
                    queryCalculateImage = false;
                    calculateImage = false;
                }
                
                // Si chaine de caractères modifiée, on redessine avec le texte à jour
                if (stringModified) {
                    SDL_RenderCopy(renderer, fractalTexture, NULL, NULL);
                    sprintf(displayBuffer, "Veuillez entrer le nouveau nombre maximal d'itérations: %s", inputBuffer);
                    render_text(renderer, font, displayBuffer, 10, 10, ORIGIN_UP_LEFT);
                    SDL_RenderPresent(renderer);
                }
            }
        }

        // Si on est en attente du dessin de la fractale
        /*
        if (fractalCalcPending) {
        
        }
        */

        if (queryCalculateImage && activateAutoRefresh) {
            calculateImage = true;
        }

        // Si l'utilisateur n'a pas encore cliqué après le redimensionnement, on ne redessine pas la fractale
        if (!initialClickDone) {
            // Afficher texture centrée uniquement
            SDL_SetRenderTarget(renderer, NULL);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);

            int texW, texH;
            SDL_QueryTexture(fractalTexture, NULL, NULL, &texW, &texH);

            SDL_Rect dstRect;
            dstRect.w = texW;
            dstRect.h = texH;
            dstRect.x = (windowWidth - texW) / 2;
            dstRect.y = (windowHeight - texH) / 2;

            SDL_RenderCopy(renderer, fractalTexture, NULL, &dstRect);
            render_text(renderer, font, "Cliquez pour réactiver...", windowWidth / 2, windowHeight / 2, ORIGIN_MIDDLE_CENTER);
            SDL_RenderPresent(renderer);
            redrawInterface = false;
            queryCalculateImage = false;
            calculateImage = false;
            
            drawingMade = true;
        }



        // Si on modifie la vue et qu'on demande un recalcul, ou qu'on force un recalcul
        if (calculateImage) {

            // Sélectionne l'écran comme cible            
            SDL_SetRenderTarget(renderer, NULL);

            // Imprime la texture du Mandelbrot correctement placée par rapport au zoom et à l'offset
            draw_mandelbrot_well_placed(renderer, fractalTexture, windowWidth, windowHeight, zoom, lastZoom, lastOffsetX, lastOffsetY, offsetX, offsetY);
            
            // Pour afficher le message du chargement
            render_text(renderer, font, "Chargement en cours...", windowWidth / 2, windowHeight / 2, ORIGIN_MIDDLE_CENTER);
            
            // Affiche le résultat pour prévenir du chargement
            SDL_RenderPresent(renderer);
            
            
            task.max_iteration = max_iteration;
            task.zoom = zoom;
            task.offsetX = offsetX;
            task.offsetY = offsetY;
            task.width = windowWidth;
            task.height = windowHeight;
            task.antialiasing = activateAntialiasing;

            SDL_Thread* thread = SDL_CreateThread(calculate_iterations, "CalcFractalThread", &task);

            int thread_return;
            SDL_WaitThread(thread, &thread_return);
            
            // Sélectionne la texture comme cible SDL
            SDL_SetRenderTarget(renderer, fractalTexture);
            
            // Avec les itérations calculées, on fait maintenant le rendu en couleur sur la texture
            render_iterations(renderer, task.iterationMap, windowWidth, windowHeight, palette, max_iteration, task.actual_max, activateAntialiasing);
            
            // Dessine la texture sur l'écran, comme elle vient d'être redessinnée pas besoin de l'ajuster auparavant
            SDL_RenderCopy(renderer, fractalTexture, NULL, NULL);
            
            // Sauvegarde les dernière valeurs de zoom et d'offset
            lastZoom = zoom;
            lastOffsetX = offsetX;
            lastOffsetY = offsetY;
            
            drawingMade = true;
        }

        // Si on demande à redessiner le tout
        if (redrawInterface || calculateImage) {
        
            SDL_SetRenderTarget(renderer, NULL);

            // Imprime la texture du Mandelbrot correctement placée par rapport au zoom et à l'offset
            draw_mandelbrot_well_placed(renderer, fractalTexture, windowWidth, windowHeight, zoom, lastZoom, lastOffsetX, lastOffsetY, offsetX, offsetY);
        
            // Pour afficher les messages
            if (!hideInterface) {
            
                // Calculer l'espacement vertical proportionnel à la hauteur de la fenêtre
                float verticalSpacing = 15 + windowWidth * 0.01f; // Par exemple, 3% de la hauteur de la fenêtre

                // Paramètres de l'image, bord bas gauche
                sprintf(displayBuffer, "Nombre d'itérations max: %d", max_iteration);
                render_text(renderer, font, displayBuffer, 10, windowHeight - verticalSpacing, ORIGIN_UP_LEFT);
                sprintf(displayBuffer, "Zoom actuel: %f", zoom);
                render_text(renderer, font, displayBuffer, 10, windowHeight - 2 * verticalSpacing, ORIGIN_UP_LEFT);
                sprintf(displayBuffer, "Offset actuel: X: %f   Y: %f", offsetX, offsetY);
                render_text(renderer, font, displayBuffer, 10, windowHeight - 3 * verticalSpacing, ORIGIN_UP_LEFT);

                // Controles, bord bas droite
                #ifdef __linux__
                    if (advancedMode) {
                        render_text(renderer, font, "M pour toggle précision Normale/Haute:   HAUTE", windowWidth - 10, windowHeight - 9 * verticalSpacing, ORIGIN_UP_RIGHT);
                    } else {
                        render_text(renderer, font, "M pour toggle précision Normale/Haute: NORMALE", windowWidth - 10, windowHeight - 9 * verticalSpacing, ORIGIN_UP_RIGHT);
                    }
                #endif

                if (activateAntialiasing) {
                    render_text(renderer, font, "J pour toggle l'antialiasing:  ON", windowWidth - 10, windowHeight - 1 * verticalSpacing, ORIGIN_UP_RIGHT);
                } else {
                    render_text(renderer, font, "J pour toggle l'antialiasing: OFF", windowWidth - 10, windowHeight - 1 * verticalSpacing, ORIGIN_UP_RIGHT);
                }

                if (activateAutoRefresh) {
                    render_text(renderer, font, "R pour toggle l'autorefresh:  ON", windowWidth - 10, windowHeight - 2 * verticalSpacing, ORIGIN_UP_RIGHT);
                } else {
                    render_text(renderer, font, "R pour toggle l'autorefresh: OFF", windowWidth - 10, windowHeight - 2 * verticalSpacing, ORIGIN_UP_RIGHT);
                }

                switch (colorScheme) {
                    case HOT_COLD:
                        render_text(renderer, font, "B pour alterner les couleurs: CHAUD/FROID", windowWidth - 10, windowHeight - 3 * verticalSpacing, ORIGIN_UP_RIGHT);
                        break;
                    case WHITE_BLACK:
                        render_text(renderer, font, "B pour alterner les couleurs:  BLANC/NOIR", windowWidth - 10, windowHeight - 3 * verticalSpacing, ORIGIN_UP_RIGHT);
                        break;
                }
                
                render_text(renderer, font, "Clic droit/gauche glissé & flèches directionnelles pour naviguer", windowWidth - 10, windowHeight - 4 * verticalSpacing, ORIGIN_UP_RIGHT);
                render_text(renderer, font, "I pour entrer une nouvelle valeur d'itérations max", windowWidth - 10, windowHeight - 5 * verticalSpacing, ORIGIN_UP_RIGHT);
                render_text(renderer, font, "Espace pour recalculer l'image du Mandelbrot", windowWidth - 10, windowHeight - 6 * verticalSpacing, ORIGIN_UP_RIGHT);
                render_text(renderer, font, "Clic droit simple pour retour en arrière", windowWidth - 10, windowHeight - 7 * verticalSpacing, ORIGIN_UP_RIGHT);
                render_text(renderer, font, "H pour toggle l'interface", windowWidth - 10, windowHeight - 8 * verticalSpacing, ORIGIN_UP_RIGHT);
                render_text(renderer, font, "W: Zoom  X: OffsetX  C: OffsetY", windowWidth - 10, windowHeight - 9 * verticalSpacing, ORIGIN_UP_RIGHT);
            }
            
            drawingMade = true;
        }

        
        // Si on sélectionne une zone
        if (rightDragging) {

            // Sélectionne l'écran comme cible
            SDL_SetRenderTarget(renderer, NULL);

            // Imprime la texture du Mandelbrot correctement placée par rapport au zoom et à l'offset
            draw_mandelbrot_well_placed(renderer, fractalTexture, windowWidth, windowHeight, zoom, lastZoom, lastOffsetX, lastOffsetY, offsetX, offsetY);
            
            // Dessine le rectangle blanc de sélection
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_Rect rect;
            rect.x = smallest(selectStart.x, selectEnd.x);
            rect.y = smallest(selectStart.y, selectEnd.y);
            rect.w = abs(selectEnd.x - selectStart.x);
            rect.h = abs(selectEnd.y - selectStart.y);
            SDL_RenderDrawRect(renderer, &rect);
        }
        
        // Si on a dessiné sur l'écran, l'afficher
        if (drawingMade) {
            SDL_SetRenderTarget(renderer, NULL);
            SDL_RenderPresent(renderer);
            drawingMade = false;
        }
        
        // On réinitialise les demandes de dessin
        redrawInterface = false;
        queryCalculateImage = false;
        calculateImage = false;
        
        firstExecution = false;
        
        // Evite enormément de lag en ne consommant pas trop de cycles processeur
        SDL_Delay(10);
    }

    free(task.iterationMap);

    // Ferme les polices d'écriture
    TTF_CloseFont(font);
    TTF_Quit();

    // Ferme SDL et libère la mémoire
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_DestroyTexture(fractalTexture);
    SDL_Quit();
    
    return 0;
}




// Génère une palette de couleur allant dans couleurs froides au couleurs chaudes
void generate_palette_hot_cold() {
    for (int i = 0; i < PALETTE_SIZE; i++) {
        float t = (float)i / (PALETTE_SIZE - 1);
        palette[i].r = (int)(9 * (1 - t) * t * t * t * 255);         // rouge
        palette[i].g = (int)(15 * (1 - t) * (1 - t) * t * t * 255);  // vert
        palette[i].b = (int)(8.5 * (1 - t) * (1 - t) * (1 - t) * t * 255); // bleu
    }
}

// Génère une palette de couleur allant dans couleurs de noir à blanc
void generate_palette_white_black() {
    for (int i = 0; i < PALETTE_SIZE; i++) {
        float t = (float)i / (PALETTE_SIZE - 1);
        palette[i].r = (int)(255 * t); // rouge
        palette[i].g = (int)(255 * t); // vert
        palette[i].b = (int)(255 * t); // bleu
    }
}


// Ajoute la vue actuelle a l'historique
void push_view(double zoom, double offsetX, double offsetY) {
    if (historyIndex < MAX_HISTORY - 1) {
        historyIndex++;
        history[historyIndex].zoom = zoom;
        history[historyIndex].offsetX = offsetX;
        history[historyIndex].offsetY = offsetY;
    }
}

// Retire une vue de l'historique et la mettre dans le zoom et l'offset actuel
bool pop_view(double *zoom, double *offsetX, double *offsetY) {
    if (historyIndex >= 0) {
        *zoom = history[historyIndex].zoom;
        *offsetX = history[historyIndex].offsetX;
        *offsetY = history[historyIndex].offsetY;
        historyIndex--;
        return true;
    }
    return false;
}



// Calcule le nombre d'itérations de chaque pixels
int calculate_iterations(void* arg) {
    FractalTask* task = (FractalTask*)arg;

    int w = task->width;
    int h = task->height;
    int total = w * h;
    int done = 0;

    task->actual_max = 0;

    task->finished = false;
    task->progress = 0;

    for (int py = 0; py < h; py++) {
        for (int px = 0; px < w; px++) {
            double x0 = (px - w / 2.0) / task->zoom + task->offsetX;
            double y0 = (py - h / 2.0) / task->zoom + task->offsetY;

            double x = 0.0, y = 0.0;
            int iteration = 0;

            while (x * x + y * y <= 4.0 && iteration < task->max_iteration) {
                double xtemp = x * x - y * y + x0;
                y = 2.0 * x * y + y0;
                x = xtemp;
                iteration++;
            }

            task->iterationMap[py * w + px] = iteration;
            if (iteration > task->actual_max)
                task->actual_max = iteration;

            done++;
        }
        // Mettre à jour la progression une fois par ligne
        task->progress = (done * 100) / total;
    }

    task->finished = true;
    return 0;
}


// Fait le rendu en couleurs des itérations sur la cible SDL
void render_iterations(SDL_Renderer *renderer, int *iterationMap, int w, int h, SDL_Color *palette, int max_iteration, int actual_max, bool antialiasing) {

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
            if (iteration == max_iteration) {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            } else {
                int colorIndex = (iteration * (PALETTE_SIZE - 1)) / actual_max;
                SDL_Color color = palette[colorIndex];
                SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
            }
            SDL_RenderDrawPoint(renderer, px, py);
        }
    }
}


// Dessine la texture du Mandelbrot dans un objet texture SDL, version avec un zoom virtuellement infini
// Disponible seulement dans la version linux
#ifdef __linux__
    void draw_mandelbrot_high_precision(SDL_Renderer *renderer, SDL_Texture *texture, int max_iteration, double zoom, double offsetX, double offsetY, int width, int height, bool antialiasing) {

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        mpfr_t x0, y0, x, y, xtemp, tempX, tempY, two;
        int actual_max = 0;

        // Calcul du facteur de précision basé sur le zoom
        double min_precision_bits = 10.0; // Précision minimale (par exemple 100 bits)
        double scaling_factor = 4.0;      // Facteur d'échelle pour la précision
        double precision_bits = min_precision_bits + (scaling_factor * log(zoom));  // Formule logarithmique

        // Initialisation de la précision
        mpfr_init2(x0, (int)precision_bits);  // 256 bits de précision
        mpfr_init2(y0, (int)precision_bits);
        mpfr_init2(x, (int)precision_bits);
        mpfr_init2(y, (int)precision_bits);
        mpfr_init2(xtemp, (int)precision_bits);
        mpfr_init2(tempX, (int)precision_bits);
        mpfr_init2(tempY, (int)precision_bits);
        mpfr_init2(two, (int)precision_bits);
        
        // Contient juste la valeur 2
        mpfr_set_d(two, (double)(2.0), MPFR_RNDN);

        // 1ère passe : calcul des itérations et stockage des valeurs pour chaque pixel
        int *iterationMap = malloc(width * height * sizeof(int));  // Utilisation d'un tableau à une dimension
        for (int py = 0; py < height; py++) {
            for (int px = 0; px < width; px++) {
                // Convertir les coordonnées de l'écran en coordonnées fractale avec MPFR
                mpfr_set_d(x0, (px - width / 2.0) / zoom + offsetX, MPFR_RNDN);
                mpfr_set_d(y0, (py - height / 2.0) / zoom + offsetY, MPFR_RNDN);

                mpfr_set_d(x, 0.0, MPFR_RNDN);
                mpfr_set_d(y, 0.0, MPFR_RNDN);
                int iteration = 0;

                mpfr_t norm;
                mpfr_init2(norm, (int)precision_bits);  // Initialisation de la variable norme

                while (iteration < max_iteration) {
                    // Calcul de x² + y² (norme quadratique)
                    mpfr_mul(tempX, x, x, MPFR_RNDN);  // tempX = x^2
                    mpfr_mul(tempY, y, y, MPFR_RNDN);  // tempY = y^2
                    mpfr_add(norm, tempX, tempY, MPFR_RNDN);  // norm = x^2 + y^2

                    // Si la norme dépasse 4, on quitte la boucle
                    if (mpfr_cmp_d(norm, 4.0) >= 0.0) {
                        break;
                    }

                    // Calcul du prochain x et y
                    mpfr_sub(xtemp, tempX, tempY, MPFR_RNDN);  // xtemp = x^2 - y^2
                    mpfr_add(xtemp, xtemp, x0, MPFR_RNDN);     // xtemp += x0

                    mpfr_mul(tempX, x, y, MPFR_RNDN);          // tempX = x * y
                    mpfr_mul(tempX, tempX, two, MPFR_RNDN);    // tempX *= 2
                    mpfr_add(y, tempX, y0, MPFR_RNDN);         // y = 2xy + y0

                    mpfr_set(x, xtemp, MPFR_RNDN);  // Mise à jour de x

                    iteration++;
                }
                
                iterationMap[py * width + px] = iteration;
                if (iteration > actual_max) {
                    actual_max = iteration;
                }
            }
        }

        // 2ème passe : rendu avec interpolation pour l'anticrénelage
        for (int py = 0; py < height; py++) {
            for (int px = 0; px < width; px++) {
                int iteration;

                if (antialiasing) {
                    // Antialiasing activé : moyenne avec les pixels voisins
                    int neighboringIterations = iterationMap[py * width + px];
                    int count = 1;

                    if (px > 0) {
                        neighboringIterations += iterationMap[py * width + (px - 1)];
                        count++;
                    }
                    if (px < width - 1) {
                        neighboringIterations += iterationMap[py * width + (px + 1)];
                        count++;
                    }
                    if (py > 0) {
                        neighboringIterations += iterationMap[(py - 1) * width + px];
                        count++;
                    }
                    if (py < height - 1) {
                        neighboringIterations += iterationMap[(py + 1) * width + px];
                        count++;
                    }

                    iteration = neighboringIterations / count;
                } else {
                    // Antialiasing désactivé : on utilise la valeur brute
                    iteration = iterationMap[py * width + px];
                }

                // On va sélectionner la couleur de chaque pixel depuis la palette pré-générée
                if (iteration == max_iteration) {
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                } else {
                    int colorIndex = (iteration * (PALETTE_SIZE - 1)) / actual_max;
                    SDL_Color color = palette[colorIndex];
                    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
                }
                SDL_RenderDrawPoint(renderer, px, py);
            }
        }

        free(iterationMap);  // Libération de la mémoire

        // Libération des variables MPFR
        mpfr_clear(x0);
        mpfr_clear(y0);
        mpfr_clear(x);
        mpfr_clear(y);
        mpfr_clear(xtemp);
        mpfr_clear(tempX);
        mpfr_clear(tempY);
        mpfr_clear(two);
    }
#endif


// transforme les coordonnée de l'écran en coordonnée de fractale
void screen_to_fractal(int x, int y, double zoom, double offsetX, double offsetY, int width, int height, double *fx, double *fy) {
    *fx = (x - width / 2) / zoom + offsetX;
    *fy = (y - height / 2) / zoom + offsetY;
}


// Fonction pour calculer les rectangles source et destination
void calculate_rectangles(double fx1, double fy1, double fx2, double fy2, 
                           int windowWidth, int windowHeight, double zoom, double lastZoom, 
                           double lastOffsetX, double lastOffsetY, double offsetX, double offsetY, 
                           SDL_Rect *srcRect, SDL_Rect *destRect) {
                           
    // Et retransforme ces points en pixels de la texture précédente (avec lastZoom)
    int sx1, sy1, sx2, sy2;
    sx1 = (int)((fx1 - lastOffsetX) * lastZoom + windowWidth / 2);
    sy1 = (int)((fy1 - lastOffsetY) * lastZoom + windowHeight / 2);
    sx2 = (int)((fx2 - lastOffsetX) * lastZoom + windowWidth / 2);
    sy2 = (int)((fy2 - lastOffsetY) * lastZoom + windowHeight / 2);

    *srcRect = (SDL_Rect){ smallest(sx1, sx2), smallest(sy1, sy2), abs(sx2 - sx1), abs(sy2 - sy1) };
    
    // Clamp le srcRect à l'intérieur de la texture
    if (srcRect->x < 0) {
        srcRect->w += srcRect->x;
        srcRect->x = 0;
    }
    if (srcRect->y < 0) {
        srcRect->h += srcRect->y;
        srcRect->y = 0;
    }
    if (srcRect->x + srcRect->w > windowWidth) {
        srcRect->w = windowWidth - srcRect->x;
    }
    if (srcRect->y + srcRect->h > windowHeight) {
        srcRect->h = windowHeight - srcRect->y;
    }

    
    // Taille à dessiner à l'écran (plus petite que l'écran)
    destRect->w = (int)(windowWidth * (zoom / lastZoom));
    destRect->h = (int)(windowHeight * (zoom / lastZoom));

    // Positionner le rectangle en tenant compte du déplacement du centre
    int shiftX = (int)((lastOffsetX - offsetX) * zoom);
    int shiftY = (int)((lastOffsetY - offsetY) * zoom);

    destRect->x = (windowWidth - destRect->w) / 2 + shiftX;
    destRect->y = (windowHeight - destRect->h) / 2 + shiftY;

    // Clamp le srcRect à l'intérieur de la texture
    if (destRect->x < 0) {
        destRect->w += destRect->x;
        destRect->x = 0;
    }
    if (destRect->y < 0) {
        destRect->h += destRect->y;
        destRect->y = 0;
    }
    if (destRect->x + destRect->w > windowWidth) {
        destRect->w = windowWidth - destRect->x;
    }
    if (destRect->y + destRect->h > windowHeight) {
        destRect->h = windowHeight - destRect->y;
    }
}


// Appelle toute les fonctions nécéssaire a l'affichage de la texture proportionnel au zoom et au coordonnées
void draw_mandelbrot_well_placed(SDL_Renderer *renderer, SDL_Texture *texture, int windowWidth, int windowHeight, double zoom, double lastZoom, 
                           double lastOffsetX, double lastOffsetY, double offsetX, double offsetY) {

    // Calcule les coins dans l'ancien espace fractal (avant zoom)
    double fx1, fy1, fx2, fy2;
    screen_to_fractal(0, 0, zoom, offsetX, offsetY, windowWidth, windowHeight, &fx1, &fy1);
    screen_to_fractal(windowWidth, windowHeight, zoom, offsetX, offsetY, windowWidth, windowHeight, &fx2, &fy2);
    
    // Calcule les dimensions du rectangle de début et de destination, pour savoir comment zoomer sur la texture existante
    SDL_Rect srcRect, destRect;
    calculate_rectangles(fx1, fy1, fx2, fy2, windowWidth, windowHeight, zoom, lastZoom, lastOffsetX, lastOffsetY, offsetX, offsetY, &srcRect, &destRect);
    
    // Efface l'écran en noir
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // noir opaque
    SDL_RenderClear(renderer);
    
    // Imprime la texture bien placée sur la cible sélectionnée avant la fonction
    SDL_RenderCopy(renderer, texture, &srcRect, &destRect);
}


// Dessine du texte blanc avec un fond gris arrondi semi-transparent
void render_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, OriginType origin) {
    SDL_Color color = {255, 255, 255, 255}; // Texte blanc
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

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






