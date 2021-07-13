#version 120

uniform mat4 modelToCameraMatrix;
uniform mat4 cameraToClipMatrix;
uniform mat4 modelToWorldMatrix;
uniform mat4 modelToClipMatrix;

uniform int active_lights_n; // Number of active lights (< MG_MAX_LIGHT)
uniform vec3 scene_ambient;  // rgb
uniform float u_time; // Tiempo

uniform struct light_t {
	vec4 position;    // Camera space
	vec3 diffuse;     // rgb
	vec3 specular;    // rgb
	vec3 attenuation; // (constant, lineal, quadratic)
	vec3 spotDir;     // Camera space
	float cosCutOff;  // cutOff cosine
	float exponent;
} theLights[4];     // MG_MAX_LIGHTS

uniform struct material_t {
	vec3  diffuse;
	vec3  specular;
	float alpha;
	float shininess;
} theMaterial;

attribute vec3 v_position; // Model space
attribute vec3 v_normal;   // Model space
attribute vec2 v_texCoord;

varying vec4 f_color;
varying vec2 f_texCoord;




//ley de lambert diapos 14 a 17 dot de los vectores normal y luz
float lambert_factor(vec3 n, const vec3 l){
	//si es negativa se devuelve 0
	float NdotL = dot(n,l);
	//float res = max(0,NdotL);
	return max(0.0,NdotL);
}



// diapos 20 a 24      normal			luz			camara	brillo
float specular_factor(const vec3 n, const vec3 l, const vec3 v, float m){

		float NdotL = dot(n,l);
		//22
		vec3 r =normalize(2 * NdotL * n - l);
		float RdotV = dot(r, v);
		if((RdotV > 0.0) && (m > 0.0)){
			float specularLight =  pow(RdotV, m);
			return specularLight;
		}
		return 0.0;
}

//////////////////


// diapos 30-32 pdf iluminacion 2 no da mucha info


void direction_light(const in int light, const in vec3 lightDir,const in vec3 viewDir,const in vec3 normal, inout vec3 diffuse, inout vec3 specular) {

	float NdotL = lambert_factor(normal,lightDir);
	if(NdotL > 0.0){
		//la luz difusa sera la combinacion de todas ellas
		diffuse = diffuse + NdotL * theMaterial.diffuse *theLights[light].diffuse;
		
		//calculamos la especular
		float specularF = specular_factor(normal,lightDir,viewDir,theMaterial.shininess);
		if(specularF > 0.0){
			//si es mayor que 0 hacemos la combinacion
			specular = specular + NdotL * specularF * theMaterial.specular * theLights[light].specular;
		}
	}
}


void point_light(const in int light, const in vec3 pos, in vec3  viewDir, in vec3  normal, inout vec3 diffuse, inout vec3 specular) {

	// distancia euclidia entre dos puntos
	float dist = length(theLights[light].position.xyz - pos);

	//el vector entre dos puntos se calcula haciendo la resta entre estos
	vec3 lightDir = normalize(theLights[light].position.xyz - pos);

	// se calcula en funcion a la distancia
	float atenuacion = (theLights[light].attenuation.x + theLights[light].attenuation.y * dist + theLights[light].attenuation.z * dist * dist);

	if(atenuacion > 0.0){
		//si es mayor que 0 hacemos el calculo normal
		atenuacion = 1/atenuacion;
	}
	else{
		// si no para evitar infinitos o casos raros lo ponemos a 1
		atenuacion = 1;
	}

	float NdotL = lambert_factor(normal, lightDir); 
	if(NdotL > 0.0){

		//la luz difusa es otra vez la combinacion de los valores difusos
		diffuse = diffuse + atenuacion * NdotL * theMaterial.diffuse * theLights[light].diffuse;

		float specularF = specular_factor(normal, lightDir, viewDir, theMaterial.shininess);
		if (specularF > 0.0){
			//si es mayor que 0 hacemos la combinacion de los valores especulares
			specular = specular + NdotL * atenuacion * specularF  * theMaterial.specular * theLights[light].specular;
		}
	}
}



void spot_light(const in int light, const in vec3 pos, const in vec3 viewDir, const in vec3 normal, inout vec3 diffuse, inout vec3 specular) {
	// Otra vez clavado al pervertex
	vec3 ligthDir = normalize(theLights[light].position.xyz - pos);
	float SdotL = dot(-ligthDir ,normalize(theLights[light].spotDir));

	if (SdotL >= theLights[light].cosCutOff){

		float NdotL = lambert_factor(normal, ligthDir);

		if(NdotL > 0.0){

			float cSpot = pow(max(SdotL, 0.0), theLights[light].exponent);
			diffuse = diffuse + NdotL * theMaterial.diffuse * theLights[light].diffuse * cSpot;
			float specularF = specular_factor(normal, ligthDir, viewDir, theMaterial.shininess);

			if (specularF > 0.0){

				specular = specular + NdotL * specularF * theMaterial.specular * theLights[light].specular * cSpot;

			}
		}
	}
}




void main() {

	// los inicialamos a 0
	vec3 lDifusa = vec3(0.0);
	vec3 lEspecular = vec3(0.0);
	

	// para conseguir nuestra normal cogemos la v_normal
	vec3 normal = normalize((modelToCameraMatrix * vec4(v_normal, 0.0)).xyz);


	// v_position pasada a 4 dimensiones y cambiandola a sistema de referencia de la camara nos da la posicion del ojo
	vec4 eyePos = modelToCameraMatrix * vec4(v_position,1.0);

	

	//el vector de direccion de vista es la direccion del ojo negadam y normalizada
	vec3 viewDir =normalize(-eyePos.xyz);

	vec3 lightDir;
	//inicializar las luces
	for(int light=0; light < active_lights_n; ++light) {
		if(theLights[light].position.w == 0.0) {
		  // direction light
			lightDir = -normalize( theLights[light].position.xyz); //En la direccional, el vector posición ya es el vector dirección
			direction_light(light, lightDir, viewDir, normal, lDifusa, lEspecular);
		} else if (theLights[light].cosCutOff == 0.0){
			// point light
			point_light(light, eyePos.xyz, viewDir, normal, lDifusa, lEspecular);
			
		} else {
			// spot light
			spot_light(light, eyePos.xyz, viewDir, normal, lDifusa, lEspecular);
		  	}
		}
	
	//alteraciones para el color segun la luz
	f_color = vec4(scene_ambient + lDifusa + lEspecular, 1.0);
	gl_Position = modelToClipMatrix * vec4(v_position, 1.0);
	f_texCoord = v_texCoord;

}
