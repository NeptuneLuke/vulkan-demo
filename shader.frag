#version 450

layout(location = 0) out vec4 out_color;

void main() {

	out_color = vec4(1.0, 0.0, 0.0, 1.0);
}

/*	
The triangle that is formed by the positions from the vertex shader fills
an area on the screen with fragments. The fragment shader is invoked on these
fragments to produce a color and depth for the framebuffer(s).
This fragment shader outputs the color red on the entire triangle.

The main function is called for every fragment just like the vertex shader
main function is called for every vertex.

Colors in GLSL are vec4 objects with RGBA channels within [0,1] range.

Unlike gl_Position in the vertex shader, there is no built-in variable to output
a color for the current fragment. You have to specify your own output variable for each framebuffer
where the layout(location = 0) modifier specifies the index of the framebuffer.
The color red is written to the out_color variable that is linked to the first (and only)
framebuffer at index 0.
*/