// Very simple sphere with triangular surface mesh
// Created for GMSH

SetFactory("OpenCASCADE");

// Create sphere
Sphere(1) = {0, 0, 0, 1.0};