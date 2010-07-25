# Make a gradient
# The blockiness is just a side effect of the tiling system.

size x y;

h = x * y;

c = 1.0 / h;

r = r + c;
g = g + c;
b = b + c;
a = 1.0;

output r g b a;
