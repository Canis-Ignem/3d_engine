#version 120

uniform int active_lights_n; // Number of active lights (< MG_MAX_LIGHT)
uniform vec3 scene_ambient; // Scene ambient light

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

uniform sampler2D texture0;

varying vec3 f_position;      // camera space
varying vec3 f_viewDirection; // camera space
varying vec3 f_normal;        // camera space
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

void direction_light(const in int light, const in vec3 lightDir, const in vec3 viewDir, const in vec3 normal, inout vec3 diffuse, inout vec3 specular) {
	//Identico al pervertex pero como me confundi y empece por el perfragment estaba fallando todo el rato
	//por que los errores que corregi en el pervertex no los habia apsado de vuelta aqui 
	float NdotL = lambert_factor(normal, lightDir);

	if(NdotL > 0.0){

		diffuse = diffuse + NdotL * theMaterial.diffuse * theLights[light].diffuse;
		float specularF = specular_factor(normal, lightDir, viewDir, theMaterial.shininess);

		if (specularF > 0.0){
			specular = specular + NdotL * specularF * theMaterial.specular * theLights[light].specular;
		}
	}
}

// Note: do not calculate the attenuation in point_lights

void point_light(const in int light, const in vec3 pos, const in vec3 viewDir, const in vec3 normal, inout vec3 diffuse, inout vec3 specular) {
	//Tambien muy parecida pero sin atenuacion
	vec3 lightDir = normalize(theLights[light].position.xyz - pos);
	float NdotL = lambert_factor(normal, lightDir);

	float dist = length(theLights[light].position.xyz - pos);
	float atenuacion = (theLights[light].attenuation.x + theLights[light].attenuation.y * dist + theLights[light].attenuation.z * dist * dist);

	if(atenuacion > 0.0){
		//si es mayor que 0 hacemos el calculo normal
		atenuacion = 1/atenuacion;
	}
	else{
		// si no para evitar infinitos o casos raros lo ponemos a 1
		atenuacion = 1;
	}
	

	if(NdotL > 0.0){
		diffuse = diffuse + atenuacion * NdotL * theMaterial.diffuse * theLights[light].diffuse;
		float specularF = specular_factor(normal, lightDir, viewDir, theMaterial.shininess);
		if (specularF > 0.0){
			specular = specular + NdotL * atenuacion * specularF * theMaterial.specular * theLights[light].specular;
		}
	}
}

void spot_light(const in int light, const in vec3 pos, const in vec3 viewDir, const in vec3 normal, inout vec3 diffuse, inout vec3 specular) {
	// Otra vez clavado al pervertex
	
	vec3 lightDir  = normalize(theLights[light].position.xyz - pos);
	float SdotL = dot(normalize(theLights[light].spotDir), -lightDir);

	if (SdotL >= theLights[light].cosCutOff){

		float NdotL = lambert_factor(normal, lightDir);

		if(NdotL > 0.0){

			float cSpot = pow(max(SdotL, 0.0), theLights[light].exponent);
			diffuse = diffuse + NdotL * theMaterial.diffuse * theLights[light].diffuse * cSpot;
			float specularF = specular_factor(normal, lightDir, viewDir, theMaterial.shininess);

			if (specularF > 0.0){

				specular = specular + NdotL * specularF * theMaterial.specular * theLights[light].specular * cSpot;

			}
		}
	}
}

void main() {

	//El main en general es casi igual al pervertex
	vec3 lDifusa = vec3(0.0);
	vec3 lEspecular = vec3(0.0);

	vec3 normal = normalize(f_normal);

	vec3 viewDir = normalize(f_viewDirection);

	vec3 lightDir; 


	for(int light=0; light < active_lights_n; ++light) {

		//Los criterios son los mismos que en el pervertex
		if(theLights[light].position.w == 0.0) {

			lightDir = -normalize( theLights[light].position.xyz); 
			direction_light(light, lightDir, viewDir, normal, lDifusa, lEspecular);

		} else {

		  	if (theLights[light].cosCutOff == 0.0) {

				point_light(light, f_position, viewDir, normal, lDifusa, lEspecular);

		  	} else {

				spot_light(light, f_position, viewDir, normal, lDifusa, lEspecular);
				
		  	}
		}
	}

	//tenemos que aplicarles a las texturas los cambios que le ahcen al color las luces
	gl_FragColor = vec4(scene_ambient + lDifusa + lEspecular, 1.0) * texture2D(texture0, f_texCoord);
}
