#!/usr/bin/env python3
# Maze BFS visualizer - animates BFS visits and final path from a JSON file
import sys
import json
import time
import os
import warnings
import contextlib
import math

os.environ.setdefault("PYGAME_HIDE_SUPPORT_PROMPT", "1")
warnings.filterwarnings("ignore", message=r"pkg_resources is deprecated as an API.*", category=UserWarning)
try:
    
    with open(os.devnull, 'w') as devnull:
        with contextlib.redirect_stdout(devnull), contextlib.redirect_stderr(devnull):
            import pygame
except Exception:
    print("Error: pygame is required to run the visualizer.\nInstall it in your virtualenv (pip install pygame).")
    sys.exit(2)


def load_json(path):
    # Load JSON file containing maze and steps
    with open(path, 'r') as f:
        return json.load(f)


def main(path):
    # Main entry: load data and prepare visualization
    data = load_json(path)
    maze = data['maze']
    height = len(maze)
    width = len(maze[0]) if height > 0 else 0

    steps = data.get('steps', [])
    start = tuple(data.get('start', [0,0]))
    end = tuple(data.get('end', [width-1, height-1]))

    # Initialize Pygame and create window
    pygame.init()
    margin = 24
    max_canvas = 900
    
    cell_size_w = max(8, (max_canvas - margin * 2) // max(1, width))
    cell_size_h = max(8, (max_canvas - margin * 2) // max(1, height))
    cell_size = max(12, min(48, min(cell_size_w, cell_size_h)))

    win_w = width * cell_size + margin * 2
    win_h = height * cell_size + margin * 2
    screen = pygame.display.set_mode((win_w, win_h))
    pygame.display.set_caption('Maze BFS visualization')

    clock = pygame.time.Clock()

    bg = (0, 0, 0)
    cell_bg = bg
    cell_edge = (20, 20, 20)
    wall_col = (255, 255, 255)
    path_col = (255, 190, 70)
    start_col = (90, 210, 90)
    end_col = (210, 90, 90)
    visit_surf = pygame.Surface((cell_size, cell_size), pygame.SRCALPHA)

    # Draw maze grid and walls
    def draw_maze():
        screen.fill(bg)
        radius = max(3, cell_size // 8)
        wall_thick = max(2, cell_size // 10)
        
        for y in range(height):
            for x in range(width):
                rx = margin + x * cell_size
                ry = margin + y * cell_size
                rect = pygame.Rect(rx, ry, cell_size, cell_size)
                pygame.draw.rect(screen, cell_bg, rect, border_radius=radius)
                pygame.draw.rect(screen, cell_edge, rect, width=1, border_radius=radius)

                cell = maze[y][x]
                
                if cell.get('up', True):
                    pygame.draw.line(screen, wall_col, (rx, ry), (rx + cell_size, ry), wall_thick)
                if cell.get('down', True):
                    pygame.draw.line(screen, wall_col, (rx, ry + cell_size), (rx + cell_size, ry + cell_size), wall_thick)
                if cell.get('left', True):
                    pygame.draw.line(screen, wall_col, (rx, ry), (rx, ry + cell_size), wall_thick)
                if cell.get('right', True):
                    pygame.draw.line(screen, wall_col, (rx + cell_size, ry), (rx + cell_size, ry + cell_size), wall_thick)

    visited_order = []          
    visit_index = {}            
    path_set = []
    path_anim = []

    path_anim_duration = 0.14   
    visit_anim = {}                
    visit_anim_duration = 0.28     
    frontier_size = 14             
    visit_alpha_faint = 90         
    visit_alpha_front = 200        
    visit_pulse_col = (60, 130, 255)   
    visit_square_col = visit_pulse_col = (60, 130, 255)

    idx = 0
    running = True
    paused = False
    fps = 60
    step_delay = 0.02  

    last_step_time = time.time()
    last_frame_time = last_step_time

    # Main loop: handle events, advance steps, and render animations
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_SPACE:
                    paused = not paused

        now = time.time()
        
        dt = now - last_frame_time
        last_frame_time = now

        # Advance to next recorded step when not paused
        if not paused and idx < len(steps) and now - last_step_time >= step_delay:
            s = steps[idx]
            t = s.get('type', 'visit')
            x = s['x']
            y = s['y']
            if t == 'visit':
                if (x, y) not in visit_index:
                    visit_index[(x, y)] = len(visited_order)
                    visited_order.append((x, y))
                    
                    visit_anim[(x, y)] = 0.0
            elif t == 'path':
                path_set.append((x, y))
                
                path_anim.append({'x': x, 'y': y, 'p': 0.0})
            idx += 1
            last_step_time = now

        # Redraw maze background and walls
        draw_maze()

        # Draw animated visit pulses
        if visit_anim:
            remove_anim = []
            for key in list(visit_anim.keys()):
                px, py = key
                p = visit_anim[key]
                p = min(1.0, p + (dt / visit_anim_duration))
                visit_anim[key] = p
                
                ease = math.sin(p * math.pi)
                alpha = int(visit_alpha_front * ease)
                size = max(3, int(cell_size * (0.5 + 0.4 * ease)))
                visit_surf.fill((0, 0, 0, 0))
                rect = pygame.Rect(0, 0, size, size)
                rect.center = (cell_size // 2, cell_size // 2)
                pygame.draw.rect(visit_surf, (*visit_pulse_col, alpha), rect, border_radius=max(2, size // 6))
                rx = margin + px * cell_size
                ry = margin + py * cell_size
                screen.blit(visit_surf, (rx, ry))
                if p >= 1.0:
                    remove_anim.append(key)
            for k in remove_anim:
                visit_anim.pop(k, None)

        # Draw visited squares (frontier + faint older visits)
        if visited_order:
            current_visit_idx = len(visited_order) if idx >= len(steps) else idx
            frontier_threshold = max(0, current_visit_idx - frontier_size)
            
            for (x, y), v_idx in visit_index.items():
                
                if (x, y) in visit_anim:
                    continue
                rx = margin + x * cell_size
                ry = margin + y * cell_size
                if v_idx >= frontier_threshold:
                    alpha = visit_alpha_front
                    size = max(3, int(cell_size * 0.6))
                else:
                    alpha = visit_alpha_faint
                    size = max(2, int(cell_size * 0.4))
                visit_surf.fill((0, 0, 0, 0))
                rect = pygame.Rect(0, 0, size, size)
                rect.center = (cell_size // 2, cell_size // 2)
                pygame.draw.rect(visit_surf, (*visit_square_col, alpha), rect, border_radius=max(2, size // 6))
                screen.blit(visit_surf, (rx, ry))

        remove_path_idx = []
        for i, entry in enumerate(path_anim):
            entry['p'] = min(1.0, entry['p'] + (dt / path_anim_duration))
            p = entry['p']
            x = entry['x']
            y = entry['y']
            rx = margin + x * cell_size
            ry = margin + y * cell_size
            
            pad = int((1.0 - p) * (cell_size // 2))
            rect = pygame.Rect(rx + pad, ry + pad, cell_size - pad * 2, cell_size - pad * 2)
            pygame.draw.rect(screen, path_col, rect, border_radius=max(2, pad//2))
            if p >= 1.0:
                remove_path_idx.append(i)

        for idx_rm in reversed(remove_path_idx):
            path_anim.pop(idx_rm)

        # Draw final path squares (static after animation)
        for (x, y) in path_set:
            
            if any((e['x'] == x and e['y'] == y) for e in path_anim):
                continue
            rx = margin + x * cell_size
            ry = margin + y * cell_size
            pr = max(2, cell_size // 6)
            rect = pygame.Rect(rx + pr, ry + pr, cell_size - pr * 2, cell_size - pr * 2)
            pygame.draw.rect(screen, path_col, rect, border_radius=max(2, pr//2))

        sx = margin + start[0] * cell_size
        sy = margin + start[1] * cell_size
        ex = margin + end[0] * cell_size
        ey = margin + end[1] * cell_size
        scx = sx + cell_size // 2
        scy = sy + cell_size // 2
        ecx = ex + cell_size // 2
        ecy = ey + cell_size // 2
        outer_r = max(4, cell_size // 3)
        pygame.draw.circle(screen, start_col, (scx, scy), outer_r)
        pygame.draw.circle(screen, (255, 255, 255), (scx, scy), outer_r, width=2)
        pygame.draw.circle(screen, end_col, (ecx, ecy), outer_r)
        pygame.draw.circle(screen, (255, 255, 255), (ecx, ecy), outer_r, width=2)

        # Present frame
        pygame.display.flip()
        clock.tick(fps)

    pygame.quit()


if __name__ == '__main__':
    if len(sys.argv) > 1:
        p = sys.argv[1]
    else:
        p = 'maze_bfs_steps.json'
    if not os.path.exists(p):
        print(f"JSON file '{p}' not found.")
        sys.exit(1)
    main(p)

