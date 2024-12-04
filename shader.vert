#version 450

vec2 positions[3] = vec2[](
	
	vec2(0.0, -0.5),
	vec2(0.5, 0.5),
	vec2(-0.5, 0.5)
);

vec3 colors[3] = vec3[](
	
	vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

layout(location = 0) out vec3 fragment_color;

void main() {

	gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
	fragment_color = colors[gl_VertexIndex];
}

/*	
We are doing something unorthodox: include the coordinates directly inside the vertex shader.

gl_VertexIndex variable: contains the index of the current vertex
	usually it is an index into the vertex buffer, but in our case
	it will be an index into a hardcoded array of vertex data
  
	the position of each vertex is accessed from the constant array
	in the shader and combined with dummy z and w components to produce
	a position in clip coordinates

gl_Position variable: functions as the output

Now the triangle is a gradient of colors. To do this we just need
to add a new vector of colors, with every component being a vector of RGB colors
for every vertex of the triangle.
Now we need to pass the colors vector to the fragment shader, and we do this
by adding an output for the colors vector and add a matching input in the fragment shader.
*/