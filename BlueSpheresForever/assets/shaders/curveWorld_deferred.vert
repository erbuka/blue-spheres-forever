#version 330

uniform mat4 in_ModelMatrix;
uniform mat4 in_ViewMatrix;
uniform mat4 in_ProjectionMatrix;

uniform mat4 in_ShadowViewMatrix;
uniform mat4 in_ShadowProjectionMatrix;

uniform vec4 in_ProjectionPlaneNormal;
uniform vec4 in_ProjectionPlaneTangent;
uniform vec4 in_ProjectionCenter;

uniform bool in_NormalMapping;

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Normal;
layout(location = 2) in vec2 in_TexCoord;

smooth out vec4 position;
smooth out vec3 normal;
smooth out vec2 texCoord;
smooth out vec4 positionShadow;
smooth out mat3 TBNInverse;

vec3 calcProjectedPosition(mat4 viewMatrix, inout vec3 normal) {
	
	vec4 positionVCS = viewMatrix * in_ModelMatrix * vec4(in_Position, 1.0);
	
	vec4 projectionCenterVCS = viewMatrix * in_ProjectionCenter;
	
	vec3 centerToPositionVCS = (positionVCS - projectionCenterVCS).xyz;
	normal = normalize(centerToPositionVCS);
	
	vec3 projectedPosition = projectionCenterVCS.xyz + normal * dot(centerToPositionVCS, (viewMatrix * in_ProjectionPlaneNormal).xyz);
	
	return projectedPosition;
}

void main() {
	
	vec3 N;
	
	vec3 projectedPositionLCS = calcProjectedPosition(in_ShadowViewMatrix, N);
	vec3 projectedPositionVCS = calcProjectedPosition(in_ViewMatrix, N);	
	
	gl_Position = in_ProjectionMatrix * vec4(projectedPositionVCS, 1.0);
	
	if(!in_NormalMapping) {
		normal = (in_ViewMatrix * in_ModelMatrix * vec4(in_Normal, 0.0)).xyz;		
	} else {
		mat3 TBN;
		vec3 T, B;
		
		B = normalize(cross(N, (in_ViewMatrix * in_ProjectionPlaneTangent).xyz));
		T = normalize(cross(N, B));
		
		TBNInverse = mat3(T, B, N);
	}
	
	position = in_ProjectionMatrix * vec4(projectedPositionVCS, 1.0);
	texCoord = in_TexCoord;
	positionShadow = in_ShadowProjectionMatrix * vec4(projectedPositionLCS, 1.0);
}