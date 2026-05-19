import pygame
import sys

# Colors
COLOR_BG = (25, 25, 25)
COLOR_S = (43, 92, 101)        # Teal
COLOR_I = (255, 107, 98)       # Red
COLOR_R = (130, 130, 130)      # Grey
COLOR_D = (15, 15, 15)         # Black
COLOR_ZONE_BORDER = (60, 60, 60)
COLOR_CONFINE_RING = (255, 215, 0)

STATUS_COLORS = {0: COLOR_S, 1: COLOR_I, 2: COLOR_R, 3: COLOR_D}

GRID_SIZE = 2000  
SCALE = 0.45       
WINDOW_SIZE = int(GRID_SIZE * SCALE)

pygame.init()
screen = pygame.display.set_mode((WINDOW_SIZE, WINDOW_SIZE))
pygame.display.set_caption("SIR Multiverse Mapping Simulator")
clock = pygame.time.Clock()

# Open file handle to stream lines efficiently
try:
    file_handle = open("output/agent_positions.txt", "r")
except FileNotFoundError:
    print("Error: 'agent_positions.txt' not found! Run C program first.")
    sys.exit()

current_step = 0
running = True
next_line = file_handle.readline()

while running:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False

    screen.fill(COLOR_BG)

    # Draw region boxes
    pygame.draw.rect(screen, COLOR_ZONE_BORDER, (0, 0, int(1000 * SCALE), int(1000 * SCALE)), 1)
    pygame.draw.rect(screen, COLOR_ZONE_BORDER, (int(1500 * SCALE), int(1500 * SCALE), int(500 * SCALE), int(500 * SCALE)), 1)

    # Read and render all agents belonging to the current step
    agents_rendered = 0
    while next_line:
        parts = next_line.strip().split(',')
        if len(parts) != 6:
            next_line = file_handle.readline()
            continue
            
        step_num, _, x, y, status, confined = map(int, parts)
        
        # If this line belongs to a future step, break out and render the current frame
        if step_num > current_step:
            break
            
        # Draw agent
        screen_x = int(x * SCALE)
        screen_y = int(y * SCALE)
        radius = 3 if status != 3 else 1
        pygame.draw.circle(screen, STATUS_COLORS[status], (screen_x, screen_y), radius)
        
        if confined and status != 3:
            pygame.draw.circle(screen, COLOR_CONFINE_RING, (screen_x, screen_y), radius + 2, 1)
            
        agents_rendered += len(parts)
        next_line = file_handle.readline()

    # If we finished reading the step data, advance the frame counter
    if not next_line and agents_rendered == 0:
        # Loop finished, reset to beginning or stay at end
        pass
    else:
        # Step increment calculation match C outputs
        current_step += 5 

    # UI Panel Layout Info
    font = pygame.font.SysFont("Consolas", 18)
    text_surface = font.render(f"Simulating Time Step: {current_step}", True, (240, 240, 240))
    screen.blit(text_surface, (15, 15))

    pygame.display.flip()
    clock.tick(10) # Playback velocity speed control (10 FPS)

file_handle.close()
pygame.quit()
sys.exit()