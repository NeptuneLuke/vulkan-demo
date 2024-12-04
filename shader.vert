#version 450

vec2 positions[3] = vec2[](
	
	vec2(0.0, -0.5),
	vec2(0.5, 0.5),
	vec2(-0.5, 0.5)
);

void main() {

	gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
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
*/