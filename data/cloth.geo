// Parameters
n = 20; // subdivisions
lc = 1.0 / n; // mesh size

// Define the square domain (unit square)
Point(1) = {0, 0, 0, lc};
Point(2) = {1, 0, 0, lc};
Point(3) = {1, 1, 0, lc};
Point(4) = {0, 1, 0, lc};

Line(1) = {1, 2};
Line(2) = {2, 3};
Line(3) = {3, 4};
Line(4) = {4, 1};

Line Loop(1) = {1, 2, 3, 4};
Plane Surface(1) = {1};

// Structured mesh (transfinite)
Transfinite Line {1, 3} = n + 1;
Transfinite Line {2, 4} = n + 1;
Transfinite Surface {1};
Recombine Surface {1}; // This converts triangles to quads

// Mesh settings
Mesh.ElementOrder = 1; // Use linear elements
Mesh.RecombinationAlgorithm = 1; // Use Blossom algorithm for recombination (optional)
Mesh.RecombineAll = 1; // Optional: recombine all surfaces automatically