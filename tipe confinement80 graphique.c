#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <string.h> // Ajouté pour memcpy

typedef struct Agent {
    int id;
    int pos_x;
    int pos_y;
    int infected_steps;
    bool S;
    bool I;
    bool R;
    bool D;
    bool C;
    bool A;
} Agent;

typedef struct Simulation {
    Agent* agents;
    int num_agents;
    int time_steps;
    int infection_radius;
    int recover_probability;
    int infection_probability;
    int death_probability;
    int confinement_threshold;
    bool threshold_reached;
    bool confinement_active;

    // --- NOUVEAUX CHAMPS POUR L'OPTIMISATION ---
    
    // 1. Double Buffering : Évite de faire malloc/free à chaque étape
    Agent* new_agents; 
    
    // 2. Variables de la Grille Spatiale (Spatial Hash)
    int grid_cell_size; // La taille d'une cellule (égale au rayon d'infection)
    int grid_width;     // Le nombre de cellules sur une ligne (ex: 1000 / 40)
    int num_cells;      // Le nombre total de cellules (width * width)
    
    // Les deux tableaux du Spatial Hash
    int* cell_head;  // Contient l'ID du premier agent dans chaque cellule
    int* agent_next; // Contient l'ID de l'agent suivant dans la même cellule

} Simulation;

Simulation* create_simulation(int num_agents, int time_steps, int infection_radius, int recover_probability, int infection_probability, int death_probability, int confinement_threshold, bool confinement_active) {
    Simulation* sim = (Simulation*)malloc(sizeof(Simulation));
    sim->agents = (Agent*)malloc(num_agents * sizeof(Agent));
    sim->num_agents = num_agents;
    sim->time_steps = time_steps;
    sim->infection_radius = infection_radius;
    sim->recover_probability = recover_probability;
    sim->infection_probability = infection_probability;
    sim->death_probability = death_probability;
    sim->confinement_threshold = confinement_threshold;
    sim->threshold_reached = false;
    sim->confinement_active = confinement_active;

    // --- INITIALISATION DE L'OPTIMISATION ---
    
    // On alloue new_agents UNE SEULE FOIS au début
    sim->new_agents = (Agent*)malloc(num_agents * sizeof(Agent));

    // La taille de la cellule s'adapte à ton rayon variable
    sim->grid_cell_size = (infection_radius > 0) ? infection_radius : 10;
    
    // On ajoute +1 pour s'assurer que les agents aux bords (x=999) ont bien une cellule
    sim->grid_width = (1000 / sim->grid_cell_size) + 1;
    sim->num_cells = sim->grid_width * sim->grid_width;

    // On alloue les tableaux de notre "classeur" spatial
    sim->cell_head = (int*)malloc(sim->num_cells * sizeof(int));
    sim->agent_next = (int*)malloc(num_agents * sizeof(int));

    // Initialize agents
    for (int i = 0; i < num_agents; i++) {
        sim->agents[i].id = i;
        sim->agents[i].pos_x = rand() % 1000;
        sim->agents[i].pos_y = rand() % 1000;
        sim->agents[i].S = true;
        sim->agents[i].I = false;
        sim->agents[i].R = false;
        sim->agents[i].D = false;
        sim->agents[i].C = false;
        sim->agents[i].infected_steps = 0;
        sim->agents[i].A = false; // Initialisé pour éviter des bugs de mémoire
    }

    return sim;
}

void destroy_simulation(Simulation* sim) {
    // N'oublie pas de libérer les nouveaux tableaux pour éviter les fuites de mémoire
    free(sim->new_agents);
    free(sim->cell_head);
    free(sim->agent_next);
    free(sim->agents);
    free(sim);
}

int* get_state(Simulation* sim, int* state) {
    int count_S = 0, count_I = 0, count_R = 0, count_D = 0;
    for (int i = 0; i < sim->num_agents; i++) {
        if (sim->agents[i].S) count_S++;
        else if (sim->agents[i].I && !(sim->agents[i].C)) count_I++;
        else if (sim->agents[i].R) count_R++;
        else if (sim->agents[i].D) count_D++;
    }
    state[0] = count_S;
    state[1] = count_I;
    state[2] = count_R;
    state[3] = count_D;
    return state;
}

int distance(Agent* a, Agent* b) {
    return abs(a->pos_x - b->pos_x) + abs(a->pos_y - b->pos_y);
}

int max_int(int a, int b) {
    return (a > b) ? a : b;
}

void movement(Agent* agent, Agent* new_agent) {
    if (rand () % 100 < 30) { 
        new_agent->pos_x = agent->pos_x + (rand() % 9) - 4;
        new_agent->pos_y = agent->pos_y + (rand() % 9) - 4;
    }
    
    if (!(agent->C)) {
        if (new_agent->pos_x < 0) new_agent->pos_x = 0;
        if (new_agent->pos_x > 999) new_agent->pos_x = 999;
        if (new_agent->pos_y < 0) new_agent->pos_y = 0;
        if (new_agent->pos_y > 999) new_agent->pos_y = 999; 
    } else {
        new_agent->pos_x = agent->pos_x;
        new_agent->pos_y = agent->pos_y;
    }
}

void step(Simulation* sim) {
    // POURQUOI : On copie la mémoire d'un coup (très rapide) au lieu de faire malloc/free
    memcpy(sim->new_agents, sim->agents, sim->num_agents * sizeof(Agent));

    // --- 1. RÉINITIALISER LA GRILLE ---
    // POURQUOI : On vide le classeur à chaque étape (la valeur -1 signifie "vide")
    for (int i = 0; i < sim->num_cells; i++) {
        sim->cell_head[i] = -1;
    }

    // --- 2. PLACER LES AGENTS DANS LA GRILLE ---
    // POURQUOI : On range chaque agent dans le bon dossier une seule fois
    for (int i = 0; i < sim->num_agents; i++) {
        // Les morts et confinés (qui sont à 1750, hors de la carte) ne transmettent pas
        // On les ignore pour éviter qu'ils fassent planter les limites de la grille
        if (sim->agents[i].D || sim->agents[i].C) continue; 

        int cx = sim->agents[i].pos_x / sim->grid_cell_size;
        int cy = sim->agents[i].pos_y / sim->grid_cell_size;
        
        // Sécurité pour rester dans les limites du tableau
        if (cx < 0) cx = 0; else if (cx >= sim->grid_width) cx = sim->grid_width - 1;
        if (cy < 0) cy = 0; else if (cy >= sim->grid_width) cy = sim->grid_width - 1;
        
        int cell_idx = cy * sim->grid_width + cx;
        
        // On ajoute l'agent au début de la liste chaînée de sa cellule
        sim->agent_next[i] = sim->cell_head[cell_idx];
        sim->cell_head[cell_idx] = i;
    }

    // --- 3. TRAITEMENT ---
    for (int i = 0; i < sim->num_agents; i++) {
        movement(&sim->agents[i], &sim->new_agents[i]);
        
        if (sim->agents[i].I) {
            sim->new_agents[i].infected_steps++;
            
            if (sim->agents[i].infected_steps >= 30 && !sim->agents[i].C) {
                
                // On trouve dans quelle cellule se trouve l'infecté
                int my_cx = sim->agents[i].pos_x / sim->grid_cell_size;
                int my_cy = sim->agents[i].pos_y / sim->grid_cell_size;

                // POURQUOI : On boucle UNIQUEMENT sur les 9 cellules locales
                for (int dx = -1; dx <= 1; dx++) {
                    for (int dy = -1; dy <= 1; dy++) {
                        int nx = my_cx + dx;
                        int ny = my_cy + dy;
                        
                        // Si la cellule voisine est bien dans la carte
                        if (nx >= 0 && nx < sim->grid_width && ny >= 0 && ny < sim->grid_width) {
                            int cell_idx = ny * sim->grid_width + nx;
                            
                            // On parcourt la liste chaînée des agents présents dans cette cellule
                            int target = sim->cell_head[cell_idx];
                            while (target != -1) {
                                // On vérifie la distance uniquement avec ces agents proches
                                if (sim->agents[target].S && distance(&sim->agents[i], &sim->agents[target]) <= sim->infection_radius) {
                                    if (rand() % 100 < sim->infection_probability) {
                                        if (rand() % 100 < 0) { // Ton code d'origine avait 0 ici (20% en commentaire)
                                            sim->new_agents[target].A = true;
                                        }
                                        sim->new_agents[target].S = false;
                                        sim->new_agents[target].I = true;
                                    }
                                }
                                // On passe à l'agent suivant dans la même cellule
                                target = sim->agent_next[target];
                            }
                        }
                    }
                }

                // Logique de guérison / mort / confinement (intact)
                if (rand() % 100 < sim->recover_probability) {
                    sim->new_agents[i].I = false;
                    sim->new_agents[i].R = true;
                }
                else if (rand() % 100 < sim->death_probability) {
                    sim->new_agents[i].I = false;
                    sim->new_agents[i].R = false; 
                    sim->new_agents[i].D = true; 
                }
                
                if (! (sim->agents[i].A) && !(sim->agents[i].C) && sim->threshold_reached && sim->agents[i].infected_steps >= 15 && sim->confinement_active){ 
                    sim->new_agents[i].C = true;
                    sim->new_agents[i].pos_x = 1750;  
                    sim->new_agents[i].pos_y = 1750;
                }
            }
        }
    }

    // --- 4. SWAP DES POINTEURS ---
    // POURQUOI : Au lieu de recopier ou de faire free(), on échange juste les étiquettes.
    Agent* temp = sim->agents;
    sim->agents = sim->new_agents;
    sim->new_agents = temp;
}

void run_simulation(Simulation* sim) {
    int steps = 0;
    int* state = (int*)malloc(4 * sizeof(int));
    int max_i = 0;

    for (int i = 0; i < 10; i++) {
        sim->agents[i].S = false;
        sim->agents[i].I = true;
    }
    get_state(sim, state);
    max_i = state[1];

    FILE* file = fopen("simulation_data.csv", "w");
    FILE* pos_file = fopen("agent_positions.txt", "w");
    if (file == NULL || pos_file == NULL) {
        printf("Error opening tracking data files!\n");
        free(state);
        return;
    }
    
    fprintf(file, "Step,Susceptible,Infected,Recovered,Dead\n");

    while (state[1] > 0 && steps < sim->time_steps) {
        fprintf(file, "%d,%d,%d,%d,%d\n", steps, state[0], state[1], state[2], state[3]);
        
        if (steps % 5 == 0) {
            for (int i = 0; i < sim->num_agents; i++) {
                int status = 0; 
                if (sim->agents[i].D) status = 3;
                else if (sim->agents[i].I) status = 1;
                else if (sim->agents[i].R) status = 2;
                fprintf(pos_file, "%d,%d,%d,%d,%d,%d\n", steps, sim->agents[i].id, sim->agents[i].pos_x, sim->agents[i].pos_y, status, sim->agents[i].C ? 1 : 0);
            }
        }

        if (steps % 100 == 0) {
            printf("Steps : %d, Susceptible : %d, Infected : %d, Recovered : %d, Dead : %d\n", steps, state[0], state[1], state[2], state[3]);
        }
        
        if (state[1] > 50) { 
            sim->threshold_reached = true;
        } 
        
        step(sim);
        state = get_state(sim, state);
        max_i = max_int(max_i, state[1]);
        steps = steps + 1;
    }
    
    fclose(file);
    fclose(pos_file);

    printf("\n=== RESULTATS ===\n");
    printf("Steps totaux: %d\n", steps);
    printf("Infections max : : %d\n", max_i);
    printf("Final  -> Susceptible: %d, Infecte: %d, Recupere: %d, Mort: %d\n", state[0], state[1], state[2], state[3]);
    printf("Data exported successfully!\n");
    free(state);
}

int main() {
    int recover_probability;
    int infection_probability;
    int population_size;
    int time_steps;
    int infection_radius;
    int death_probability;
    int confinement_threshold;
    bool confinement_active;
    int custom;
    printf("Utiliser des valeurs spéciales? (0 pour défaut, 1 pour spécial): ");
    scanf("%d", &custom);
    if (custom) {
        printf("Recup : (0-100): ");
        scanf("%d", &recover_probability); 
        printf("Infection : (0-100): ");
        scanf("%d", &infection_probability); 
        printf("Mortalité : (0-100): ");
        scanf("%d", &death_probability); 
        printf("population: ");
        scanf("%d", &population_size); 
        printf("infection");
        scanf("%d", &infection_radius); 
        printf("Activer le confinement? (0 , 1): ");
        scanf("%d", &confinement_active);
    } else {
        recover_probability = 2;
        infection_probability = 8;
        death_probability = 0;
        population_size = 1000;
        time_steps = 1000;    
        infection_radius = 40;   
        confinement_threshold = 50;
        confinement_active = true;
    }
    
    srand(time(NULL));
    Simulation* sim = create_simulation(population_size, time_steps, infection_radius, recover_probability, infection_probability, death_probability, confinement_threshold, confinement_active);
    run_simulation(sim);
    destroy_simulation(sim);
    
    return 0;
}