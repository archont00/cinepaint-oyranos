# This script will extract a luminance matte
# If the source image doesn't have an alpha channel, give it one by going to
# the Image menu, and selecting Alpha->Add Alpha Channel

input r g b a;

# play around with these values until you get what you want
lum = r + g + b;
lum = clip(lum);

# now, multiply the image by the new matte
r = r * lum;
g = g * lum;
b = b * lum;

output r g b lum;

