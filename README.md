# Terrain Generator 
## C/C++, OpenGL, GLUT

Algorithmic terrain mesh generator. Three different algorithms are implemented amongst other features such as Gourard shading and customizable terrain complexity. Lighting and scene can all be moved - see control instructions.

Makefile included.

### Control Instructions (please take note of upper/lower case in command instructions):
- Move the camera up and down with c and v respectively, and look up and down with z and x.
- Rotate around the y axis with the up and down arrows, around the x axis with left and right arrow keys.
- Move the first light (originally at 0,0) with 'alt' + 't','f','g', or 'h', for +x, -z, -x, or +z, respectively.
- Move the second light (originally at max width, max depth) with 'alt' + 'i','j','k', or 'l', for +x, -z, -x, or +x, respectively.
- Reset the terrain (to all flat), with the 'R' key, and randomize the terrain with the 'r' key.
- Toggle wireframe view-mode with the 'w' key.
- The 't' and 'y' keys can toggle between triangle-strips and quad-strips, respectively.
- Toggle lighting in the scene with the 'L' key.
- Toggle between flat-shading and Gouraud shading with the 's' key.
- Quit the program with either the 'esc' key or the 'q' key.
- Change the terrain complexity (number algorithm iterations) with the 'C' key.
- When lighting is off, toggle topgraphic-style colouring with 'T' key.
- Toggle terrain algorithms using 'G'; toggles between circles, fault, and particle deposition.
