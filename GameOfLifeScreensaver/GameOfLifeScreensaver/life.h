#pragma once

double getRandomFloatInRange(double min, double max);
char* createSimulation(int rows, int columns, int boundary, double density);
int countNeighborhood(int rows, int columns, int x, int y, char* matrix);
char* stepSimulation(int rows, int columns, int boundary, char* prevMatrix); 
void applyBorders(int rows, int columns, char* matrix);
void convertSimulation(int* screen, int screenWidth, int color, char* simulation, int rows, int columns, int cellSize);
void specialPlot(int* screen, int screenWidth, int x, int y, int size, int color);