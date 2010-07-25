input red green blue alpha;

# Show the luminance of the image
luma = red * 0.3;
luma = luma + green * 0.59;
luma = luma + blue * 0.11;

output luma luma luma alpha;
