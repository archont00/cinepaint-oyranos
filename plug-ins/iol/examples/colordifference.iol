input r g b a;

# my lame attempt at a bluescreen-via-color-difference script
# the resulting image still requires some tweaking, but it's a good start

# The image must already have an alpha channel. If it doesn't, you can give
# it an empty one by going Image->Alpha->Add Alpha Channel

# For blue screens
b = min(g,b);
a = max(r,g);

# For green screens
#g = min(g,r);
#a = max(r,b);

r = r * a;
g = g * a;
b = b * a;

output r g b a;

