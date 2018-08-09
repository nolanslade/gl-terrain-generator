/*
Nolan Slade
Terrain Generator
*/

#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <math.h>

#ifdef __APPLE__
#  include <OpenGL/gl.h>
#  include <OpenGL/glu.h>
#  include <GLUT/glut.h>
#else
#  include <GL/gl.h>
#  include <GL/glu.h>
#  include <GL/freeglut.h>
#endif

#define CIRCLE_MIN		5 		// Minimum size of our circle for our circles algorithm
#define CIRCLE_RANGE	10 		// Range for our circle size, thus the circle will be 25 + (0 to range-1)
#define MAX_DISP 		5 		// Maximum displacement used by the terrain generation algorithms
#define VERT_SPACING	3		// Distance between vertices

/* Terrain Globals */
float *heightMap;							// Array for the height values of our terrain
float *triangleNormals;						// Array for normals used for lighting the terrain (triangle strip)
float *quadNormals;							// Array for normals used for lighting the terrain (quad strip)
float *triangleVertexNormals;				// Vertex normals (triangle-strip)
float *quadVertexNormals;					// Vertex normals (quad-strip)
int terrainWidth;							// Width of the terrain (number of vertices in x direction)
int terrainDepth;							// Depth of the terrain (number of vertices in z direction)
int terrainComplexity = 1000;				// Essentially how many times the circles algorithm will be run, user-selectable to generate different terrain styles
char wireFrameMode = 's';					// Toggle for rendering mode. 's' for solid, 'w' for wireframe, 'b' for both
char shadeMode = 'f';						// Initially flat, 'f', 'g' for gourard shading
char algorithmMode = 'c';					// Initially set to 'c' for circles algorithm, 'f' for fault algorithm, 'd' for particle deposition
bool lightsOff = false;						// Toggle for lighting, initially there is no lighting, toggle with L
char stripMode = 't';						// Toggle for strip mode; ie whether the program renders on triangle strip or quad strip mode
float maxHeight;							// Used for non-lighting grayscale colouring and for topographic colouring
float minHeight;							// Used for non-lighting grayscale colouring and for topographic colouring
float terrainRotationX = 0;					// Used to rotate the terrain
float terrainRotationY = 0;
bool topographicEnabled = false;			// Used for bonus feature: advanced topographic colouring
float baseGreen[] = {0.168, 0.388, 0.196};	// Topographic green (lowest point)

/* Camera Globals */
float camPos[ ] 	= { -10.0f, 10.0f, -10.0f };
float camUp [] 		= { 0.0f, 1, 0.0f };
float camTarget [] 	= { 0 , 10.0f, 0 };
float camSpeed 		= 10.0f;

/* Lighting Globals */
// Near corner of terrain
float light_pos0 [] = { 0, 0, 0, 1 }; 		// x,y,z, w = 1 = point light
float amb0 [4]  	= { 0.2, 0.2, 1, 1 }; 
float diff0 [4] 	= { 0, 0, 1, 1 };
float spec0 [4] 	= { 0.5, 0.5, 1, 1 }; 
float lightSpeed 	= 10.0f;

// Far corner of terrain
float light_pos1 [] = { 0, 0, 0, 1 }; 		// x,y,z, w = 1 = point light
float amb1 [4]  	= { 0.2, 1, 0.2, 1 }; 
float diff1 [4] 	= { 0, 1, 0, 1 };
float spec1 [4] 	= { 0.5, 1, 0.5, 1 }; 

// Material for the filled polygons
float whiteplastic_ambient [] 	= { 0.0f, 0.0f, 0.0f, 1.0 }; 
float whiteplastic_diffuse [] 	= { 0.55, 0.55, 0.55, 1.0 }; 
float whiteplastic_specular [] 	= { 0.70, 0.70, 0.70, 1.0 }; 
float whiteplastic_shininess 	= 0.23 * 128;

// Material for the wireframes for when the wireframes are being drawn over the filled polygons
float redplastic_ambient [] 	= { 0.0f, 0.0f, 0.0f, 1.0 };
float redplastic_diffuse [] 	= { 0.5, 0.0f, 0.0f, 1.0 };
float redplastic_specular [] 	= { 0.7, 0.6, 0.6, 1.0 };
float redplastic_shininess 		= 0.25 * 128;


/*
- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
*/


/* Returns an index mapped to the 1D array of height values */
int getIndex (int x, int z) {
	return (x * terrainDepth + z);
}

/* Returns the index of the X component of a normal vector in its array */
int getNormalIndex (int x, int z, char type, bool first) {
	// Index depends on whether we are using triangles or quads, 't' for triangle
	// If we are using triangles, the result further depends on whether or not we want
	// the first triangle associated with the vertex, or the second
	if (type == 't' && first)
		return 6 * getIndex(x,z);
	else if (type == 't')
		return 6 * getIndex(x,z) + 3;
	else 
		return 3 * getIndex(x,z);
}


/* Once the surface normals are calculated, we calculate vertex normals */
void setVertexNormals () {
	printf("Calculating vertex normals...\n");
	float vertexNormal[] = {0, 0, 0};		// Our final result for each vertex
	float one[] = {0, 0, 0};				// First vector used to calculate vertex normal
	int index1 = 0;
	float two[] = {0, 0, 0};				// Second vector used to calculate vertex normal
	int index2 = 0;
	float three[] = {0, 0, 0};				// Third vector used to calculate vertex normal
	int index3 = 0;
	float four[] = {0, 0, 0};				// Fourth vector used to calculate vertex normal
	int index4 = 0;
	float five[] = {0, 0, 0};				// Fifth vector used to calculate vertex normal
	int index5 = 0;
	float six[] = {0, 0, 0};				// Sixth vector used to calculate vertex normal
	int index6 = 0;
	float magnitude;

	// Iterate through the terrain
	// Cases are needed for all corners, all edges, and inside vertices
	int indexCounter = 0;
	for (int z = 0; z < terrainDepth; z++) {
		for (int x = 0; x < terrainWidth; x++) {
			// Corners
			if (x == 0 && z == 0) {
				// Triangles
				index1 = getNormalIndex(x,z,'t',true);
				index2 = getNormalIndex(x,z,'t',false);
				one[0] = triangleNormals[index1];		one[1] = triangleNormals[index1+1]; 		one[2] = triangleNormals[index1+2];
				two[0] = triangleNormals[index2];		two[1] = triangleNormals[index2+1];			two[2] = triangleNormals[index2+2];

				// Sum the vectors
				triangleVertexNormals[indexCounter] = one[0] + two[0];
				triangleVertexNormals[indexCounter+1] = one[1] + two[1];
				triangleVertexNormals[indexCounter+2] = one[2] + two[2];

				// Quads
				index1 = getNormalIndex(x,z,'y',true);
				one[0] = quadNormals[index1];			one[1] = quadNormals[index1+1]; 			one[2] = quadNormals[index1+2];

				// Sum the vectors
				quadVertexNormals[indexCounter] = one[0];
				quadVertexNormals[indexCounter+1] = one[1];
				quadVertexNormals[indexCounter+2] = one[2];
			} 
			else if (x == 0 & z == terrainDepth - 1) {
				// Triangles
				index1 = getNormalIndex(x,z-1,'t',true);
				one[0] = triangleNormals[index1];		one[1] = triangleNormals[index1+1]; 		one[2] = triangleNormals[index1+2];

				// Sum the vectors
				triangleVertexNormals[indexCounter] = one[0];
				triangleVertexNormals[indexCounter+1] = one[1];
				triangleVertexNormals[indexCounter+2] = one[2];

				// Quads
				index1 = getNormalIndex(x,z-1,'y',true);
				one[0] = quadNormals[index1];			one[1] = quadNormals[index1+1]; 			one[2] = quadNormals[index1+2];

				// Sum the vectors
				quadVertexNormals[indexCounter] = one[0];
				quadVertexNormals[indexCounter+1] = one[1];
				quadVertexNormals[indexCounter+2] = one[2];
			}
			else if (x == terrainWidth - 1 && z == 0) {
				// Triangles
				index1 = getNormalIndex(x-1,z,'t',false);
				one[0] = triangleNormals[index1];		one[1] = triangleNormals[index1+1]; 		one[2] = triangleNormals[index1+2];

				// Sum the vectors
				triangleVertexNormals[indexCounter] = one[0];
				triangleVertexNormals[indexCounter+1] = one[1];
				triangleVertexNormals[indexCounter+2] = one[2];

				// Quads
				index1 = getNormalIndex(x-1,z,'y',true);
				one[0] = quadNormals[index1];			one[1] = quadNormals[index1+1]; 			one[2] = quadNormals[index1+2];

				// Sum the vectors
				quadVertexNormals[indexCounter] = one[0];
				quadVertexNormals[indexCounter+1] = one[1];
				quadVertexNormals[indexCounter+2] = one[2];
			}
			else if (x == terrainWidth - 1 && z == terrainDepth - 1) {
				// Triangles
				index1 = getNormalIndex(x-1,z-1,'t',true);
				index2 = getNormalIndex(x-1,z-1,'t',false);
				one[0] = triangleNormals[index1];		one[1] = triangleNormals[index1+1]; 		one[2] = triangleNormals[index1+2];
				two[0] = triangleNormals[index2];		two[1] = triangleNormals[index2+1];			two[2] = triangleNormals[index2+2];

				// Sum the vectors
				triangleVertexNormals[indexCounter] = one[0] + two[0];
				triangleVertexNormals[indexCounter+1] = one[1] + two[1];
				triangleVertexNormals[indexCounter+2] = one[2] + two[2];

				// Quads
				index1 = getNormalIndex(x-1,z-1,'y',true);
				one[0] = quadNormals[index1];			one[1] = quadNormals[index1+1]; 			one[2] = quadNormals[index1+2];

				// Sum the vectors
				quadVertexNormals[indexCounter] = one[0];
				quadVertexNormals[indexCounter+1] = one[1];
				quadVertexNormals[indexCounter+2] = one[2];
			}

			// Edges
			else if (x == 0) {
				// Triangles
				index1 = getNormalIndex(x,z-1,'t',true);
				index2 = getNormalIndex(x,z,'t',false);
				index3 = getNormalIndex(x,z,'t',true);
				one[0] = triangleNormals[index1]; 		one[1] = triangleNormals[index1+1]; 		one[2] = triangleNormals[index1+2];
				two[0] = triangleNormals[index2]; 		two[1] = triangleNormals[index2+1]; 		two[2] = triangleNormals[index2+2];
				three[0] = triangleNormals[index3]; 	three[1] = triangleNormals[index3+1]; 		three[2] = triangleNormals[index3+2];

				// Sum the vectors
				triangleVertexNormals[indexCounter] = one[0] + two[0] + three[0];
				triangleVertexNormals[indexCounter+1] = one[1] + two[1] + three[1];
				triangleVertexNormals[indexCounter+2] = one[2] + two[2] + three[2];

				// Quads
				index1 = getNormalIndex(x,z-1,'y',true);
				index2 = getNormalIndex(x,z,'y',true);
				one[0] = quadNormals[index1]; 			one[1] = quadNormals[index1+1]; 			one[2] = quadNormals[index1+2];
				two[0] = quadNormals[index2]; 			two[1] = quadNormals[index2+1];				two[2] = quadNormals[index2+2];

				// Sum the vectors, and normalize the result
				quadVertexNormals[indexCounter] = one[0] + two[0];
				quadVertexNormals[indexCounter+1] = one[1] + two[1];
				quadVertexNormals[indexCounter+2] = one[2] + two[2];
			} 
			else if (z == 0) {
				// Triangles
				index1 = getNormalIndex(x-1,z,'t',false);
				index2 = getNormalIndex(x,z,'t',true);
				index3 = getNormalIndex(x,z,'t',false);
				one[0] = triangleNormals[index1]; 		one[1] = triangleNormals[index1+1]; 		one[2] = triangleNormals[index1+2];
				two[0] = triangleNormals[index2]; 		two[1] = triangleNormals[index2+1]; 		two[2] = triangleNormals[index2+2];
				three[0] = triangleNormals[index3]; 	three[1] = triangleNormals[index3+1]; 		three[2] = triangleNormals[index3+2];

				// Sum the vectors
				triangleVertexNormals[indexCounter] = one[0] + two[0] + three[0];
				triangleVertexNormals[indexCounter+1] = one[1] + two[1] + three[1];
				triangleVertexNormals[indexCounter+2] = one[2] + two[2] + three[2];

				// Quads
				index1 = getNormalIndex(x-1,z,'y',true);
				index2 = getNormalIndex(x,z,'y',true);
				one[0] = quadNormals[index1]; 			one[1] = quadNormals[index1+1]; 			one[2] = quadNormals[index1+2];
				two[0] = quadNormals[index2]; 			two[1] = quadNormals[index2+1]; 			two[2] = quadNormals[index2+2];

				// Sum the vectors, and normalize the result
				quadVertexNormals[indexCounter] = one[0] + two[0];
				quadVertexNormals[indexCounter+1] = one[1] + two[1];
				quadVertexNormals[indexCounter+2] = one[2] + two[2];
			} 
			else if (x == terrainWidth - 1) {
				// Triangles
				index1 = getNormalIndex(x-1,z-1,'t',false);
				index2 = getNormalIndex(x-1,z-1,'t',true);
				index3 = getNormalIndex(x-1,z,'t',false);
				one[0] = triangleNormals[index1]; 		one[1] = triangleNormals[index1+1]; 		one[2] = triangleNormals[index1+2];
				two[0] = triangleNormals[index2]; 		two[1] = triangleNormals[index2+1]; 		two[2] = triangleNormals[index2+2];
				three[0] = triangleNormals[index3]; 	three[1] = triangleNormals[index3+1]; 		three[2] = triangleNormals[index3+2];

				// Sum the vectors
				triangleVertexNormals[indexCounter] = one[0] + two[0] + three[0];
				triangleVertexNormals[indexCounter+1] = one[1] + two[1] + three[1];
				triangleVertexNormals[indexCounter+2] = one[2] + two[2] + three[2];

				// Quads
				index1 = getNormalIndex(x-1,z-1,'y',true);
				index2 = getNormalIndex(x-1,z,'y',true);
				one[0] = quadNormals[index1]; 			one[1] = quadNormals[index1 + 1]; 			one[2] = quadNormals[index1+2];
				two[0] = quadNormals[index2]; 			two[1] = quadNormals[index2 + 1];			two[2] = quadNormals[index2+2];

				// Sum the vectors, and normalize the result
				quadVertexNormals[indexCounter] = one[0] + two[0];
				quadVertexNormals[indexCounter+1] = one[1] + two[1];
				quadVertexNormals[indexCounter+2] = one[2] + two[2];
			} 
			else if (z == terrainDepth - 1) {
				// Triangles
				index1 = getNormalIndex(x-1,z-1,'t',true);
				index2 = getNormalIndex(x-1,z-1,'t',false);
				index3 = getNormalIndex(x,z-1,'t',true);
				one[0] = triangleNormals[index1]; 		one[1] = triangleNormals[index1+1]; 		one[2] = triangleNormals[index1+2];
				two[0] = triangleNormals[index2]; 		two[1] = triangleNormals[index2+1]; 		two[2] = triangleNormals[index2+2];
				three[0] = triangleNormals[index3]; 	three[1] = triangleNormals[index3+1]; 		three[2] = triangleNormals[index3+2];

				// Sum the vectors
				triangleVertexNormals[indexCounter] = one[0] + two[0] + three[0];
				triangleVertexNormals[indexCounter+1] = one[1] + two[1] + three[1];
				triangleVertexNormals[indexCounter+2] = one[2] + two[2] + three[2];

				// Quads
				index1 = getNormalIndex(x-1,z-1,'y',true);
				index2 = getNormalIndex(x,z-1,'y',true);
				one[0] = quadNormals[index1]; 			one[1] = quadNormals[index1 + 1]; 			one[2] = quadNormals[index1 + 2];
				two[0] = quadNormals[index2]; 			two[1] = quadNormals[index2 + 1];			two[2] = quadNormals[index2 + 2];

				// Sum the vectors, and normalize the result
				quadVertexNormals[indexCounter] = one[0] + two[0];
				quadVertexNormals[indexCounter+1] = one[1] + two[1];
				quadVertexNormals[indexCounter+2] = one[2] + two[2];
			}

			// General case for interior vertices
			else {
				// For triangles, we take the 6 surrounding face normals, and calculate the corresponding vertex normal
				index1 = getNormalIndex(x-1,z,'t',false);
				index2 = getNormalIndex(x,z,'t',true);
				index3 = getNormalIndex(x,z,'t',false);
				index4 = getNormalIndex(x-1,z-1,'t',true);
				index5 = getNormalIndex(x-1,z-1,'t',false);
				index6 = getNormalIndex(x,z-1,'t',true);
				one[0] = triangleNormals[index1]; 		one[1] = triangleNormals[index1 + 1]; 		one[2] = triangleNormals[index1 + 2];
				two[0] = triangleNormals[index2]; 		two[1] = triangleNormals[index2 + 1]; 		two[2] = triangleNormals[index2 + 2];
				three[0] = triangleNormals[index3]; 	three[1] = triangleNormals[index3 + 1]; 	three[2] = triangleNormals[index3 + 2];
				four[0] = triangleNormals[index4]; 		four[1] = triangleNormals[index4 + 1]; 		four[2] = triangleNormals[index4 + 2];
				five[0] = triangleNormals[index5]; 		five[1] = triangleNormals[index5 + 1]; 		five[2] = triangleNormals[index5 + 2];
				six[0] = triangleNormals[index6]; 		six[1] = triangleNormals[index6 + 1]; 		six[2] = triangleNormals[index6 + 2];

				// Sum the vectors, and normalize the result using the vector's magnitude
				triangleVertexNormals[indexCounter] = one[0] + two[0] + three[0] + four[0] + five[0] + six[0];
				triangleVertexNormals[indexCounter+1] = one[1] + two[1] + three[1] + four[1] + five[1] + six[1];
				triangleVertexNormals[indexCounter+2] = one[2] + two[2] + three[2] + four[2] + five[2] + six[2];

				// For quads, we take the normals of the 4 surrounding faces, and calculate the vertex normal
				index1 = getNormalIndex(x-1,z-1,'y',true);
				index2 = getNormalIndex(x,z-1,'y',true);
				index3 = getNormalIndex(x-1,z,'y',true);
				index4 = getNormalIndex(x,z,'y',true);
				one[0] = quadNormals[index1]; 			one[1] = quadNormals[index1+1]; 			one[2] = quadNormals[index1+2];
				two[0] = quadNormals[index2]; 			two[1] = quadNormals[index2+1];				two[2] = quadNormals[index2+2];
				three[0] = quadNormals[index3]; 		three[1] = quadNormals[index3+1]; 			three[2] = quadNormals[index3+2];
				four[0] = quadNormals[index4]; 			four[1] = quadNormals[index4+1]; 			four[2] = quadNormals[index4+2];

				// Sum the vectors, and normalize the result
				quadVertexNormals[indexCounter] = one[0] + two[0] + three[0] + four[0];
				quadVertexNormals[indexCounter+1] = one[1] + two[1] + three[1] + four[1];
				quadVertexNormals[indexCounter+2] = one[2] + two[2] + three[2] + four[2];
			}

			// For the newest vertex, we find the magnitudes and normalize
			magnitude = (sqrt(pow(triangleVertexNormals[indexCounter],2) + pow(triangleVertexNormals[indexCounter+1],2) + pow(triangleVertexNormals[indexCounter+2],2)));
				
			// The vector is now normalized, and ready to use
			triangleVertexNormals[indexCounter] /= magnitude;
			triangleVertexNormals[indexCounter+1] /= magnitude;
			triangleVertexNormals[indexCounter+2] /= magnitude;

			magnitude = (sqrt(pow(quadVertexNormals[indexCounter],2) + pow(quadVertexNormals[indexCounter+1],2) + pow(quadVertexNormals[indexCounter+2],2)));
				
			// The vector is now normalized, and ready to use
			quadVertexNormals[indexCounter] /= magnitude;
			quadVertexNormals[indexCounter+1] /= magnitude;
			quadVertexNormals[indexCounter+2] /= magnitude;

			// Increment the index counter for the next vertex (+3 for next [x,y,z])
			indexCounter += 3;
		}
	}
}


/* Calculates face normals for use with flat or gourard shading */
void setNormals () {
	printf("Calculating face normals...\n");

	// Vectors we need to cross to get the normal of the two nearby triangular faces and the nearby quad face
	float vecA[] = {0, 0, 0};
	float vecB[] = {0, 0, 0};
	float vecC[] = {0, 0, 0};

	// Magnitudes are used to normalize the vectors
	float magnitudeOne = 0;
	float magnitudeTwo = 0;
	float magnitudeQuad = 0;

	// Normal vectors for the two triangles we are calculating and the quad we are calculating
	float triangleOneNorm[] = {0, 0, 0};
	float triangleTwoNorm[] = {0, 0, 0};
	float quadNorm[] = {0, 0, 0};

	// Run through all faces to calculate their face normals
	int triangleNormIndex = 0;
	int quadNormIndex = 0;
	for (int i = 0; i < terrainDepth - 1; i++) {
		for (int j = 0; j < terrainWidth - 1; j++) {
			// Set the x,y,z values of the vectors
			vecA[0] = 0; vecA[1] = heightMap[getIndex(j,i+1)] - heightMap[getIndex(j,i)]; vecA[2] = VERT_SPACING * (i+1) - VERT_SPACING * i;
			vecB[0] = VERT_SPACING * (j+1) - VERT_SPACING * j; vecB[1] = heightMap[getIndex(j+1,i+1)] - heightMap[getIndex(j,i)]; vecB[2] = VERT_SPACING * (i+1) - VERT_SPACING * i;
			vecC[0] = VERT_SPACING * (j+1) - VERT_SPACING * j; vecC[1] = heightMap[getIndex(j+1, i)] - heightMap[getIndex(j,i)]; vecC[2] = 0;

			// Cross the vectors to get the normals
			// Triangle one is A X B
			triangleOneNorm[0] = (vecA[1] * vecB[2] - vecA[2] * vecB[1]);
			triangleOneNorm[1] = (vecA[2] * vecB[0] - vecA[0] * vecB[2]);
			triangleOneNorm[2] = (vecA[0] * vecB[1] - vecA[1] * vecB[0]);

			// Triangle one is B X C
			triangleTwoNorm[0] = (vecB[1] * vecC[2] - vecB[2] * vecC[1]);
			triangleTwoNorm[1] = (vecB[2] * vecC[0] - vecB[0] * vecC[2]);
			triangleTwoNorm[2] = (vecB[0] * vecC[1] - vecB[1] * vecC[0]);

			// Quad is A X C
			quadNorm[0] = (vecA[1] * vecC[2] - vecA[2] * vecC[1]);
			quadNorm[1] = (vecA[2] * vecC[0] - vecA[0] * vecC[2]);
			quadNorm[2] = (vecA[0] * vecC[1] - vecA[1] * vecC[0]);

			// Calculate the magnitudes of the two normal vectors and then normalize them
			magnitudeOne = sqrt(pow(triangleOneNorm[0],2) + pow(triangleOneNorm[1],2) + pow(triangleOneNorm[2],2));
			magnitudeTwo = sqrt(pow(triangleTwoNorm[0],2) + pow(triangleTwoNorm[1],2) + pow(triangleTwoNorm[2],2));
			magnitudeQuad = sqrt(pow(quadNorm[0],2) + pow(quadNorm[1],2) + pow(quadNorm[2],2));
			triangleOneNorm[0] /= magnitudeOne; triangleOneNorm[1] /= magnitudeOne; triangleOneNorm[2] /= magnitudeOne;
			triangleTwoNorm[0] /= magnitudeTwo; triangleTwoNorm[1] /= magnitudeTwo; triangleTwoNorm[2] /= magnitudeTwo;
			quadNorm[0] /= magnitudeQuad; quadNorm[1] /= magnitudeQuad; quadNorm[2] /= magnitudeQuad;

			// Store the results into the array of normalized normal vectors and increment the index accordingly
			triangleNormals[triangleNormIndex++] = triangleOneNorm[0]; triangleNormals[triangleNormIndex++] = triangleOneNorm[1]; triangleNormals[triangleNormIndex++] = triangleOneNorm[2];
			triangleNormals[triangleNormIndex++] = triangleTwoNorm[0]; triangleNormals[triangleNormIndex++] = triangleTwoNorm[1]; triangleNormals[triangleNormIndex++] = triangleTwoNorm[2];
			quadNormals[quadNormIndex++] = quadNorm[0]; quadNormals[quadNormIndex++] = quadNorm[1]; quadNormals[quadNormIndex++] = quadNorm[2];
		}
	}
	// Set the vertex normals for each point on the terrain
	setVertexNormals ();
}


/* Draws the terrain based on the strip mode and the wire mode */
void drawTerrain (char wireMode) {
	// Determine the polygon mode based on our global wiremode setting
	if (wireMode == 'w')
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);	// Wire frame
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);	// Normal (filled)

	// Render the terrain using the newly set polygon mode
	// Draw by going through the triangle-strip pattern across the grid for each z.
	for (int z = 0; z < terrainDepth; z++) {
		int i = z;						// Models the depth (z)
		int j = 0;						// Models the current width position (x)
		int counter = 0;				// Counter for index modification
		int vertexNormalIndex = 0;

		// Render a new strip for each z layer
		if (stripMode == 't')
			glBegin(GL_TRIANGLE_STRIP);
		else if (stripMode == 'y')
			glBegin(GL_QUAD_STRIP);
				while (1) {
					// If we're at the end of this Z layer, we break the loop to move to the next layer
					if (counter == 2 * terrainWidth) {
						break;
					} else {
						// Construct a vector for our vertex and place the vertex
						float currentV[] = {j * VERT_SPACING, heightMap[getIndex(j,i)], i * VERT_SPACING};
							
						// Determine vertex colouring
						if (topographicEnabled && wireFrameMode == 'b' && wireMode == 'w')
							glColor3f(0,0,0);

						else if (topographicEnabled)
							glColor3f(baseGreen[0] + heightMap[getIndex(j,i)]/maxHeight, baseGreen[1] + heightMap[getIndex(j,i)]/maxHeight/8, baseGreen[2]+ heightMap[getIndex(j,i)]/maxHeight/4);
						
						else if (wireFrameMode == 'b' && wireMode == 'w') 
							glColor3f(1,0,0);
							
						else {
							if (algorithmMode == 'f') {
								// Account for possible negative height values
								float difference = 0;
							
								// Add a number to avoid floating point inaccuracies
								if (minHeight < 0) 
									difference = -1 * minHeight + 10;
		
								glColor3f((heightMap[getIndex(j,i)]+difference)/(maxHeight+difference),(heightMap[getIndex(j,i)]+difference)/(maxHeight+difference),(heightMap[getIndex(j,i)]+difference)/(maxHeight+difference));
							} else {
								if (maxHeight == 0 && minHeight == 0)
									glColor3f(1.0,1.0,1.0);
								else
									glColor3f(heightMap[getIndex(j,i)]/maxHeight,heightMap[getIndex(j,i)]/maxHeight,heightMap[getIndex(j,i)]/maxHeight);
							}
						}

						// Determine which normal we are using (quad or triangle-based vertex normal)
						vertexNormalIndex = 3 * getIndex(j,i);
						if (stripMode == 't')
							glNormal3f(triangleVertexNormals[vertexNormalIndex],triangleVertexNormals[vertexNormalIndex+1],triangleVertexNormals[vertexNormalIndex+2]);
						else
							glNormal3f(quadVertexNormals[vertexNormalIndex],quadVertexNormals[vertexNormalIndex+1],quadVertexNormals[vertexNormalIndex+2]);

						// Add the vertex with the assigned normal and colouring
						glVertex3fv(currentV);

						// Modulo operator used to increment the appropriate variables
						if (counter % 2 == 0) {
							i++;
						} else {
							j++;
							i--;
						}

						// Number of vertices has increased
						counter++;
					}
				}
			glEnd();
	}
}


/* Calculates the distance between two points in 3D space */
float pointDistance (float pointOne[3], float pointTwo[3]) {
	// Using the 3D distance formula
	return (sqrt( pow((pointTwo[0]-pointOne[0]), 2) + pow((pointTwo[1]-pointOne[1]), 2) + pow((pointTwo[2]-pointOne[2]),2) ));
}


/* Generate Values for the height map */
void generateHeightValues (bool flatten) {
	//  If argument is true, we flatten the terrain (initializing, reinitializing)
	if (flatten) {
		for (int i = 0; i < terrainWidth; i++) {
			for (int j = 0; j < terrainDepth; j++) {
				// We use this structure to define a synthetic 2D array represented as a 1D array
				// and initialize all initial height values to 0
				int index = getIndex(i,j);
				heightMap[index] = 0.0f;
			}
		}

	// Cirlces Algorithm
	} else if (algorithmMode == 'c') {
		printf("Generating terrain with the circles algorithm...\n");
		// We use the circles algorithm to randomly generate our terrain
		// We run the algorithm using a random point a number of times equal to the terrain complexity
		// That is currently set (default 100 - user selectable)
		for (int i = 0; i < terrainComplexity; i++) {
			// Generate a random point on our terrain
			int randomX = rand() % terrainWidth;		// 0 to (width - 1)
			int randomZ = rand() % terrainDepth;		// 0 to (depth - 1)
			int index = getIndex(randomX, randomZ);		// Index of our random point in the height map
			int randomY = heightMap[index];				// Height corresponding to our random point

			// Get a random circle size, set the centrepoint
			int randomCircleSize = rand() % CIRCLE_RANGE + CIRCLE_MIN;
			float circleCenter[] = {(float) randomX, (float) randomY, (float) randomZ};

			// Circles algorithm
			for (int i = 0; i < terrainWidth; i++) {
				for (int j = 0; j < terrainDepth; j++) {
					int currentPointIndex = getIndex(i,j);
					float currentPoint[] = { (float) i, heightMap[currentPointIndex], (float) j };

					// Calculate point distance and use it to see whether or not we displace the point
					float pd = pointDistance(currentPoint, circleCenter) * 2 / randomCircleSize;

					if (fabs(pd) <= 1.0) {
						int randomDisp = rand() % MAX_DISP + 1;	// Displacement is 1 to MAX_DISP + 1
						heightMap[currentPointIndex] += (randomDisp / 2 + cos(pd * 3.14) * randomDisp / 2);
					}
				}
			}
		}

	// Fault Algorithm
	} else if (algorithmMode == 'f') {
		printf("Generating terrain with the fault algorithm...\n");
		// We use the fault algorithm to randomly generate our terrain
		// We run the algorithm a number of times determined by the terrain complexity currently set
		int counter = 0;
		while (counter < terrainComplexity) {
			// Pick two random points (x,z) and use a line between them to create a fault
			int randomX1 = rand() % terrainWidth;			// 0 to (width - 1)
			int randomZ1 = rand() % terrainDepth;			// 0 to (depth - 1)
			int randomX2 = rand() % terrainWidth;			// 0 to (width - 1)
			int randomZ2 = rand() % terrainDepth;			// 0 to (depth - 1)

			// Handle displacement of points
			for (int i = 0; i < terrainWidth; i++) {
				for (int j = 0; j < terrainDepth; j++) {
					// Random displacement
					float displacement = 0.3;

					// Depending on the side of the fault, displacement is either negative or positive
					if (((randomX2 - randomX1) * (j - randomZ1) - (randomZ2 - randomZ1) * (i - randomX1)) > 0)
						heightMap[getIndex(i,j)] += displacement;
					else
						heightMap[getIndex(i,j)] -= displacement;
				}
			}
			counter++;
		}

	// Particle Deposition Algorithm
	} else if (algorithmMode == 'd') {
		printf("Generating terrain with the particle deposition algorithm...\n");
		// We use the my particle deposition algorithm to randomly generate our terrain
		// Pick a random start point a total of terrainComplexity times, then build small islands around the point
		int randNum, x, z, count;
		float displacement;

		// Pick random points, and then create islands around them randomly
		int iterations = terrainComplexity;
		if (iterations > 200) {
			iterations = 5 * terrainComplexity;
		}

		for (int i = 0; i < iterations; i ++) {
			// Generate a random point on our terrain
			int randomX = rand() % terrainWidth;	// 0 to (width - 1)
			int randomZ = rand() % terrainDepth;	// 0 to (depth - 1)

			count = 0;
			while (count < 100) {
				// Use a switch statement to randomly move around to nearby points
				randNum = rand() % 4;	// 0 to 3
				
				// Switch statement handles movement between nearby, existing vertices
				switch (randNum) {
					case 0:
						if (randomX + 1 < terrainWidth) 
							randomX++;
						break;
					case 1:
						if (randomX - 1 >= 0) 
							randomX--;
						break;
					case 2:
						if (randomZ + 1 < terrainDepth) 
							randomZ++;
						break;
					case 3:
						if (randomZ - 1 >= 0) 
							randomZ--;
						break;
				}

				// Modify height at the current point randomly
				displacement = 0.3;
				int index = getIndex(randomX, randomZ);
				heightMap[index] += displacement;
				count++;
			}
		}
	}

	// Set our max and min for non-lighting colouring
	maxHeight = heightMap[0];
	minHeight = heightMap[0];

	// Only necessary to reassign the max/min if we are not flattening the terrain
	if (!flatten) {
		for (int i = 0; i < terrainDepth * terrainWidth; i++) {
			if (heightMap[i] > maxHeight)
				maxHeight = heightMap[i];
			if (heightMap[i] < minHeight)
				minHeight = heightMap[i];
		}
	}

	// Cam position modified to account for new heights
	camPos[1] = maxHeight;
	camTarget[1] = maxHeight + minHeight / 2;

	// Light positions will be modified to reflect the new heights
	light_pos0[0] = 0; light_pos0[1] = maxHeight + 50; light_pos0[2] = 0;
	light_pos1[0] = terrainWidth * VERT_SPACING; light_pos1[1] = maxHeight + 50; light_pos1[2] = terrainDepth * VERT_SPACING;
}


/* Allows the user to reset the terrain complexity */
void setTerrainComplexity () {

	// Only accept complexities less than 2000
	while (1) {
		printf("\nEnter your new (integer <= 2000) terrain complexity: \n");
		scanf("%d",&terrainComplexity);
		
		if (terrainComplexity <= 2000)
			break;

		printf("Invalid input, make sure your complexity is less than or equal to 2000.\n");
	}

	printf("\nRegeneration underway, please wait...\n");

	// Generate new height values to reflect the new complexity
	// Reset the height to all 0s first
	generateHeightValues (true);
	generateHeightValues (false);
	setNormals ();
}


/* Printing Instructions to the Terminal Window */
void printInstructions () {
	printf("\n*****************************\nWelcome to Terrain Generator!\n*****************************\n");
	printf("Written by: Nolan Slade\n");

	printf("\nControl Instructions (please take note of upper/lower case in command instructions):\n");
	printf("\t- Move the camera up and down with c and v respectively, and look up and down with z and x.\n");
	printf("\t- Rotate around the y axis with the up and down arrows, around the x axis with left and right arrow keys.\n");
	printf("\t- Move the first light (originally at 0,0) with 'alt' + 't','f','g', or 'h', for +x, -z, -x, or +z, respectively.\n");
	printf("\t- Move the second light (originally at max width, max depth) with 'alt' + 'i','j','k', or 'l', for +x, -z, -x, or +x, respectively.\n");
	printf("\t- Reset the terrain (to all flat), with the 'R' key, and randomize the terrain with the 'r' key.\n");
	printf("\t- Toggle wireframe view-mode with the 'w' key.\n");
	printf("\t- The 't' and 'y' keys can toggle between triangle-strips and quad-strips, respectively.\n");
	printf("\t- Toggle lighting in the scene with the 'L' key.\n");
	printf("\t- Toggle between flat-shading and Gouraud shading with the 's' key.\n");
	printf("\t- Quit the program with either the 'esc' key or the 'q' key.\n\n");
	printf("\nAdditional Feature Instructions (please note the upper/lower case of the commands):\n");
	printf("\t- Change the terrain complexity (number algorithm iterations) with the 'C' key.\n");
	printf("\t- When lighting is off, toggle topgraphic-style colouring with 'T' key.\n");
	printf("\t- Toggle terrain algorithms using 'G'; toggles between circles, fault, and particle deposition.\n");
}


/*
- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
*/


/* Display Function */
void display () {

	// Necessary GL operations
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Have our camera positioned and targeted where we would like, set lighting positions
	gluLookAt(camPos[0] - 100, camPos[1] + 100, camPos[2] - 100, camTarget[0] , camTarget[1] , camTarget[2], camUp[0], camUp[1], camUp[2]);

	// Material for our terrain
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  whiteplastic_ambient); 
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  whiteplastic_diffuse); 
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  whiteplastic_specular); 
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS,  whiteplastic_shininess);

	glPushMatrix();
		glRotatef(terrainRotationX, 1, 0, 0);
		glRotatef(terrainRotationY, 0, 1, 0);
		glTranslatef(-1*(terrainWidth*VERT_SPACING)/2,0,-1*(terrainDepth*VERT_SPACING)/2);

		glLightfv(GL_LIGHT0, GL_POSITION, light_pos0);
		glLightfv(GL_LIGHT1, GL_POSITION, light_pos1);

		// Draw the terrain based on our wireframe toggle
		if (wireFrameMode == 'w') {
			drawTerrain('w');
		} else if (wireFrameMode == 's') {
			drawTerrain('n');
		} else if (wireFrameMode == 'b') {
			drawTerrain('n');
			glColor3f(1,0,0);

			// If we want to show both outlines and fill, we draw the wireframes
			// with a different material so that we can see them
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  redplastic_ambient); 
			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  redplastic_diffuse); 
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  redplastic_specular); 
			glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS,  redplastic_shininess);

			// Draw the wire frames
			drawTerrain('w');
			glColor3f(1,1,1);
		}

	glPopMatrix();

	glutSwapBuffers();
}


/* Keyboard Function */
void keyboard (unsigned char key, int xIn, int yIn) {

	// Appropriate action for each key
	switch (key)
	{
		// Q and Escape can be used to exit the program
		case 'q':
		case 27:
			printf("\n*****************************\n\n");
			exit(0);
			break;

		// w key used to toggle wireframe mode
		case 'w':
			// Switch the value of the wireframe toggle 
			if (wireFrameMode == 's') 
				wireFrameMode = 'w';
			else if (wireFrameMode == 'w')  
				wireFrameMode = 'b';
			else if (wireFrameMode == 'b')
				wireFrameMode = 's';
			break;	

		// 'R' key used to flatten the terrain (reset)
		case 'R':
			generateHeightValues(true);	
			setNormals();
			break;

		// 'r' key used to generate a new random terrain
		case 'r':
			generateHeightValues(true);	
			generateHeightValues(false);
			setNormals();
			break;

		// 's' key used to toggle between shading modes, flat and gourard
		case 's':
			if (shadeMode == 'f') {
				shadeMode = 'g';
				glShadeModel(GL_SMOOTH);
			} else {
				shadeMode = 'f';
				glShadeModel(GL_FLAT);
			}
			break;

		// 'L' key used to toggle lighting
		case 'L':
			// Toggle lighting
			if (lightsOff) {
				lightsOff = false;
				glEnable(GL_LIGHTING);
				glEnable(GL_LIGHT0);
				glEnable(GL_LIGHT1);


				glLightfv(GL_LIGHT0, GL_AMBIENT, amb0); 
				glLightfv(GL_LIGHT0, GL_DIFFUSE, diff0); 
				glLightfv(GL_LIGHT0, GL_SPECULAR, spec0);

				glLightfv(GL_LIGHT1, GL_AMBIENT, amb1); 
				glLightfv(GL_LIGHT1, GL_DIFFUSE, diff1); 
				glLightfv(GL_LIGHT1, GL_SPECULAR, spec1);

			} else {
				lightsOff = true;
				glDisable(GL_LIGHTING);
				glDisable(GL_LIGHT0);
				glDisable(GL_LIGHT1);
			}
			break;

		// 'T' key used to activate topographic colouring (only if lighting disabled)
		case 'T':
			if (lightsOff && !topographicEnabled)
				topographicEnabled = true;
			else if (lightsOff && topographicEnabled) 
				topographicEnabled = false;
			break;

		// 't' used to activate triangle strip mode
		case 't':
			// If alt is active, move the light
			if (glutGetModifiers() == GLUT_ACTIVE_ALT) {
				light_pos0[0] += lightSpeed;
			}
			else 
				stripMode = 't';
			break;

		// 'y' used to active quad-strip mode
		case 'y':
			stripMode = 'y';
			break;

		// Look up
		case 'z':
			camTarget[1]+= camSpeed;
			break;

		// Look down
		case 'x':
			camTarget[1]-= camSpeed;
			break;

		// Move up
		case 'c':
			camPos[1] += camSpeed;
			break;

		// Move down
		case 'v':
			camPos[1] -= camSpeed;
			break;

		// Toggle between algorithm modes (circle 'c', fault 'f', displacement 'd')
		case 'G':
			// Set the new algorithm mode
			if (algorithmMode == 'c')
				algorithmMode = 'f';
			else if (algorithmMode == 'f')
				algorithmMode = 'd';
			else if (algorithmMode == 'd')
				algorithmMode = 'c';

			generateHeightValues(true);
			generateHeightValues(false);
			break;

		// Allow the user to change terrain complexity with the 'C' key
		case 'C':
			setTerrainComplexity();
			break;

		case 'f':
			// If alt is active, move the light
			if (glutGetModifiers() == GLUT_ACTIVE_ALT)
				light_pos0[2] -= lightSpeed;
			break;

		case 'g':
			// If alt is active, move the light
			if (glutGetModifiers() == GLUT_ACTIVE_ALT) {
				light_pos0[0] -= lightSpeed;
			} 
			break;

		case 'h':
			// If alt is active, move the light
			if (glutGetModifiers() == GLUT_ACTIVE_ALT)
				light_pos0[2] += lightSpeed;
			break;

		case 'i':
			// If alt is active, move the light
			if (glutGetModifiers() == GLUT_ACTIVE_ALT)
				light_pos1[0] += lightSpeed;
			break;

		case 'j':
			// If alt is active, move the light
			if (glutGetModifiers() == GLUT_ACTIVE_ALT)
				light_pos1[2] -= lightSpeed;
			break;

		case 'k':
			// If alt is active, move the light
			if (glutGetModifiers() == GLUT_ACTIVE_ALT)
				light_pos1[0] -= lightSpeed;
			break;

		case 'l':
			// If alt is active, move the light
			if (glutGetModifiers() == GLUT_ACTIVE_ALT)
				light_pos1[2] += lightSpeed;
			break;
	}
	glutPostRedisplay();
}

/* Used for the interactive camera */
void special (int key, int x, int y)
{
	/* Arrow key presses rotate the camera */
	switch(key) 
	{
		case GLUT_KEY_LEFT:
			//camPos[0]-=0.3;
			terrainRotationX += 5;
			break;

		case GLUT_KEY_RIGHT:
			//camPos[0]+=0.3;
			terrainRotationX -= 5;
			break;

		case GLUT_KEY_UP:
			//camPos[2] += 0.3;
			terrainRotationY += 5;
			break;

		case GLUT_KEY_DOWN:
			//camPos[2] -= 0.3;
			terrainRotationY -= 5;
			break;
	}

	glutPostRedisplay();
}


/* Frame Rate Function */
void FPS (int val) {
	// ~ 30 FPS
	glutPostRedisplay();
	glutTimerFunc(34, FPS, 0);
}


/*
- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
*/


/* Initialize Callback Functions */
void callBackInit () {
	glutTimerFunc(0, FPS, 0);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(special);
	glutDisplayFunc(display);
}


/* Initialize GL Components */
void init () {
	// Set background colour and colour
	glClearColor(0, 0, 0, 0);
	glColor3f(1, 1, 1);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glShadeModel(GL_FLAT);

	glLightfv(GL_LIGHT0, GL_AMBIENT, amb0); 
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diff0); 
	glLightfv(GL_LIGHT0, GL_SPECULAR, spec0);
	
	glLightfv(GL_LIGHT1, GL_AMBIENT, amb1); 
	glLightfv(GL_LIGHT1, GL_DIFFUSE, diff1); 
	glLightfv(GL_LIGHT1, GL_SPECULAR, spec1);

	// Set up the projection matrix and viewing
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45, 1, 1, 10000);
}


/* Main Method */
int main (int argc, char** argv) {
	// Glut initializations
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

	// Window Initialization
	glutInitWindowSize(600,600);
	glutInitWindowPosition(50, 50);
	glutCreateWindow("Terrain Generator : Nolan Slade");

	// Print the instructions
	printInstructions();

	// Prompt the user until we get a valid input
	while (1) {
		printf("\nEnter number of vertices for the terrain (min 50,50, max 300,300), in form width,depth:\n");
		scanf("%d,%d",&terrainWidth,&terrainDepth);

		// Only accept two in range integers
		if (terrainWidth >= 50 && terrainWidth <= 300 && terrainDepth >= 50 && terrainDepth <= 300) {
			break;
		}

		printf("Invalid entry. Try again.\n");
	}

	printf("Generation underway, please wait...\n");

	// Declare the initial height map and normal arrays and generate the initial terrain
	heightMap 				= new float [terrainWidth * terrainDepth];
	triangleNormals 		= new float [3 * 2 * terrainWidth * terrainDepth];
	quadNormals 			= new float [3 * terrainWidth * terrainDepth];
	quadVertexNormals 		= new float [3 * terrainWidth * terrainDepth];
	triangleVertexNormals 	= new float [3 * terrainWidth * terrainDepth];

	generateHeightValues(true);
	generateHeightValues(false);
	setNormals();

	// Modify camera target to the centre of the terrain
	camTarget[0] = (float) terrainWidth / 2;
	camTarget[2] = (float) terrainDepth / 2;

	// Initialize callback functions and depth test
	callBackInit();
	glEnable(GL_DEPTH_TEST);

	// Backface culling
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);

	// Init GL and run main loop
	init();
	glutMainLoop();

	return 0;
}