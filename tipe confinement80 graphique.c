#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>





typedef struct Agent {
    int id;
    int pos_x;
    int pos_y;
    int infected_steps; // Nombre de steps que l'agent est infecté
    bool S;// Status asymptomatique
    bool I;// Status infecté
    bool R;// Status recupéré
    bool D;// Status mort
    bool C; // Statut confinement
    bool A; // Status asymptomatique

} Agent;

typedef struct Simulation {
    Agent* agents;
    int num_agents;
    int time_steps; //nombre d'étapes max
    int infection_radius; //distance à laquelle une infection peut se produire
    int recover_probability; //probabilité de guérison à chaque étape
    int infection_probability; //probabilité d'infection à chaque étape !!(probabilité par agent, donc change en fonction de la densité de pop c'est a dire du nombre d'agents dans le rayon d'infection, donc en fonction du nomnbre d'agents ET du rayon d'infection))!!
    int death_probability; //probabilité de mourir à chaque étape
    int confinement_threshold; //seuil d'infectés pour déclencher le confinement
    bool threshold_reached; // Indique si le seuil de x  infectés a été atteint
    bool confinement_active; // Indique si les mesures de confinement sont actuellement actives

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

    // Initialize agents
    for (int i = 0; i < num_agents; i++) {
        sim->agents[i].id = i;
        sim->agents[i].pos_x = rand() % 1000; // Position initiale aleatoire
        sim->agents[i].pos_y = rand() % 1000;
        sim->agents[i].S = true; // tout les agents démarrent susceptible
        sim->agents[i].I = false;
        sim->agents[i].R = false;
        sim->agents[i].D = false;
        sim->agents[i].C = false; // All agents start as not confined
        sim->agents[i].infected_steps = 0; // Initialize infected steps to 0
    }

    return sim;
}

void destroy_simulation(Simulation* sim) {
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
        new_agent->pos_x = agent->pos_x + (rand() % 9) - 4; // +4 -4
        new_agent->pos_y = agent->pos_y + (rand() % 9) - 4; // +4 -4
        //  
    }
    
    if (!(agent->C)) {
        if (new_agent->pos_x < 0) new_agent->pos_x = 0;
        if (new_agent->pos_x > 999) new_agent->pos_x = 999;
        if (new_agent->pos_y < 0) new_agent->pos_y = 0;
        if (new_agent->pos_y > 999) new_agent->pos_y = 999; 
    }
    else{
        new_agent->pos_x = agent->pos_x;
        new_agent->pos_y = agent->pos_y; // les confinés ne se déplacent pas

    }
}
void step(Simulation* sim) {
    Agent* new_agents = (Agent*)malloc(sim->num_agents * sizeof(Agent));
    for (int i = 0; i < sim->num_agents; i++) {
        new_agents[i] = sim->agents[i]; // On copie l'état des agents dans new_agents pour pouvoir les mettre à jour simultanément
    }

    for (int i = 0; i < sim->num_agents; i++) {
        movement(&sim->agents[i], &new_agents[i]);
        if (sim->agents[i].I) {
            new_agents[i].infected_steps++;
            if (sim->agents[i].infected_steps >= 30 && !sim->agents[i].C) {
                for (int j = 0; j < sim->num_agents; j++) {
                    if (sim->agents[j].S && distance(&sim->agents[i], &sim->agents[j]) <= sim->infection_radius && rand() % 100 < sim->infection_probability) {
                        if (rand() % 100 < 0) { // 20% d'etre asymptomatique
                            new_agents[j].A = true;
                        }
                        new_agents[j].S = false;
                        new_agents[j].I = true;
                    }
                }
                if (rand() % 100 < sim->recover_probability) {
                    new_agents[i].I = false;
                    new_agents[i].R = true;
                }
                else if (rand() % 100 < sim->death_probability) {
                    new_agents[i].I = false;
                    new_agents[i].R = false; 
                    new_agents[i].D = true; 
                }
                if (! (sim->agents[i].A) && !(sim->agents[i].C) && sim->threshold_reached && sim->agents[i].infected_steps >= 15 && sim->confinement_active){ 
                    new_agents[i].C = true;
                    new_agents[i].pos_x = 1750;  // Deplacer en confinement
                    new_agents[i].pos_y = 1750;
                }
        }
        
    }
    for (int i = 0; i < sim->num_agents; i++) {
        sim->agents[i] = new_agents[i];
    }}
    free(new_agents);

    
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
        confinement_active = false;
    }
    
    srand(time(NULL));
    Simulation* sim = create_simulation(population_size, time_steps, infection_radius, recover_probability, infection_probability, death_probability, confinement_threshold, confinement_active);
    run_simulation(sim);
    destroy_simulation(sim);
    

    return 0;
}

