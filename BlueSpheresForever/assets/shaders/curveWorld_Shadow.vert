#version 330

uniform mat4 in_ViewMatrix;
uniform mat4 in_ModelMatrix;
uniform mat4 in_ProjectionMatrix;

uniform vec4 in_ProjectionPlaneNormal;
uniform vec4 in_ProjectionCenter;

layout(location = 0) in vec3 in_Position;

smooth out vec4 position;

void main() {
	vec4 positionVCS = in_ViewMatrix * in_ModelMatrix * vec4(in_Position, 1.0);
	
	vec4 projectionCenterVCS = in_ViewMatrix * in_ProjectionCenter;
	
	vec3 centerToPositionVCS = (positionVCS - projectionCenterVCS).xyz;
	vec3 n = normalize(centerToPositionVCS);
	
	vec3 projectedPositionVCS = projectionCenterVCS.xyz + n * dot(centerToPositionVCS, (in_ViewMatrix * in_ProjectionPlaneNormal).xyz);
	
	vec4 projectedPositionNCS = in_ProjectionMatrix * vec4(projectedPositionVCS, 1.0); 
	
	gl_Position = projectedPositionNCS;
	
	position = projectedPositionNCS;
}