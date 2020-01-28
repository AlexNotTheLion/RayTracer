#version 430 core
out vec4 fColor;

in vec3 vertexPos;

uniform mat4 inverseViewMatrix;
uniform mat4 inverseProjectionMatrix;

uniform sampler2D u_albedo[13];
uniform sampler2D u_roughness[13];
uniform sampler2D u_metalic[13];
uniform sampler2D u_normal[13];
uniform int widht, height;

uniform vec3 light_pos;
uniform float light_brightness;

uniform samplerCube cubemap;

int id;

#define PI 3.1415926535897932384626433832795


struct Ray{
	vec3 origin;
	vec3 direction;
};

struct intersectResult{
	vec3 pos;
	float dist;
	bool hit;
	vec4 color;
	float shinyness;
	float radius;
	int texId;
};

//vec4 backGroundColor = vec4(0.0, 0.0 ,0.0,1.0);
//vec4 backGroundColor = vec4(1.0, 1.0 ,1.0,1.0);

Ray genRay(vec3 vertexPos);
Ray reflectionRay(Ray ray, intersectResult result);
intersectResult intersect(Ray ray);
intersectResult simpleIntersect(Ray ray);
vec4 shade(intersectResult result, Ray ray);

struct Sphere
{
	vec3 pos;
	vec4 color;
	float radius;
	float shinyness;
	int texId;
};

Sphere objects[15];

struct LightSrc{
	vec3 pos;
	vec3 direction;
	vec4 color;
	float strength;
};

LightSrc light[3];

int lid;

float facing(vec3 a, vec3 b){
	if(dot(a,b) > 0.0f)
	{
		return 1.0f;
	}
	return 0.0f;
};

Ray genRay(vec3 vertexPos){//generates a ray in ndc space from screen space

	Ray ray;
	vec4 startPoint = vec4(vertexPos.x,vertexPos.y,-1,1);
	vec4 endPoint = vec4(vertexPos.x,vertexPos.y,1,1);

	startPoint = inverseProjectionMatrix * startPoint;
	startPoint = inverseViewMatrix * startPoint;
	startPoint = startPoint / startPoint.w;
	
	endPoint = inverseProjectionMatrix * endPoint;
	endPoint = inverseViewMatrix * endPoint;
	endPoint = endPoint / endPoint.w;

	ray.origin = vec3(startPoint);
	ray.direction = vec3(normalize(endPoint - startPoint));

	return ray;
}

Ray reflectionRay(Ray ray, intersectResult result){//generates a new ray from a reflected point

	//intersection point
	Ray rtn;
	vec3 intersect = ray.origin + (result.dist)*ray.direction;
	vec3 surfaceNormal = normalize(intersect - result.pos);

	vec3 newRayDir = (ray.direction)+2*(dot(-ray.direction,surfaceNormal))*surfaceNormal;

	rtn.direction = normalize(newRayDir);
	rtn.origin = (intersect + (rtn.direction*0.0001f));

	return rtn;
}

intersectResult Intersect(Ray ray)//intersection testing
{
	intersectResult rtn;
	rtn.dist = -1;
	rtn.hit = false;

	vec3 n = ray.direction;

	for(int i = 0; i < objects.length(); i++)
	{
		if(i >= id) break;
		vec3 pa = objects[i].pos - ray.origin;
		float dist = length(pa);
		float a = dot(pa,n);

		//its inside the sphere, draw blue sphere
		if(dist <= objects[i].radius)
		{ 
			//goto next object
			continue;
		}
		
		//behind camera
		if(dot(pa,n) <= 0)
		{
			//go to next object
			continue;
		}

		//d = (p-a) - ((dot(a,n)) * n)
		vec3 dVec = pa - (a*n);
		dist = length(dVec);

		if(dist <= objects[i].radius)
		{
			float x = sqrt(pow(objects[i].radius,2)-pow(dist,2));
			float hitDist = a-x;
			if(hitDist < rtn.dist || rtn.dist < 0 && hitDist != rtn.dist && hitDist > 0)
			{
			//object is hit store all information and return
				rtn.hit = true;
				rtn.dist = hitDist;
				rtn.pos = objects[i].pos;
				vec3 n = normalize(ray.direction);
				vec3 intersect = ray.origin + (hitDist)*n;
				vec3 surfaceNormal = normalize(intersect - rtn.pos);
				vec3 intPos = surfaceNormal;
				float x = 0.5 + atan(intPos.z, -intPos.x) / (2*PI);
				float y = 0.5 - asin(intPos.y) / PI;
				rtn.shinyness = texture(u_metalic[objects[i].texId], vec2(x, y)).x;
				rtn.texId = objects[i].texId;
			}
		}
	}

return rtn;
}

float DistributionGGX(vec3 N, vec3 H, float roughness)//n = surface normal, h halfway vector between surface normal and light direction, a surface roughness
{
	float a2 = roughness * roughness;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float nom    = a2;
    float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)//n = surface nornmal, v = view direction, k = roughness
{
	float k = roughness;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float k)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, k);
    float ggx2 = GeometrySchlickGGX(NdotL, k);
	
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) //cos theta = dot of surface normal and view direciton
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 diffuseLambert(vec3 albedo){

return albedo / PI;
}

vec3 getNormal(vec2 pos, int id, vec3 objpos, vec3 sN){	
	
	vec3 tangentNormal = (texture2D(u_normal[id], vec2(pos.x,pos.y)).xyz * 2.0) - 1.0;
	
	vec3 q1 = dFdx(objpos);
	vec3 q2 = dFdy(objpos);
	vec2 st1 = dFdx(pos);
	vec2 st2 = dFdy(pos);

	vec3 n = normalize(sN);
	vec3 t = normalize(q1*st2.t - q2*st1.t);
	vec3 b = -normalize(cross(n,t));

	mat3 tbn = mat3(t,b,n);


	return normalize(tbn * tangentNormal);
}

vec4 shade(intersectResult result, Ray r){//shade pixel depending on texture and lighting

		//ray direciton
		Ray ray;
		ray = r;
		vec3 n = ray.direction;
		vec3 viewDir = -ray.direction;

		vec3 intersect = ray.origin + (result.dist)*n;
		vec3 surfaceNormal = normalize(intersect - result.pos);

		vec3 intPos = surfaceNormal;
		float x = 0.5 + atan(intPos.z, -intPos.x) / (2*PI);
		float y = 0.5 - asin(intPos.y) / PI;

		vec3 albedo = pow(texture(u_albedo[result.texId], vec2(x, y)).rgb, vec3(2.2));
		float metalic = texture(u_metalic[result.texId], vec2(x, y)).r;
		float roughness = texture(u_roughness[result.texId], vec2(x, y)).r;
		vec3 normal = getNormal(vec2(x,y), result.texId, -intersect, surfaceNormal);
		vec3 f0 = albedo;
		f0 = mix(f0, albedo, metalic);

		vec3 Lo = vec3(0);

		for(int i = 0; i < light.length(); i++)
		{
			if(i >= lid) break;
			
			vec3 lightDir = normalize(light[i].pos - intersect);

			vec3 h = normalize(viewDir + lightDir);

			float dist = length(light[i].pos - intersect);

			Ray lightRay;
			intersectResult shadowResult;
			lightRay.direction = normalize(light[i].pos - intersect);
			lightRay.origin = intersect + (lightRay.direction * 0.0001f);
			shadowResult = Intersect(lightRay);
			vec4 lColor ;

			//calculate shadows
			if(shadowResult.hit){
				lColor = vec4(vec3(0), 1.0f);
			}else{
				lColor = light[i].color;
			}
			//calculations for PBR
			float attenuation = 1.0/pow(dist,2);
			vec3 radiance = vec3(lColor) * attenuation;

			float ndf = DistributionGGX(normal, h, roughness);
			float g = GeometrySmith(normal, viewDir, lightDir, roughness);
			vec3 f = fresnelSchlick(max(dot(h,viewDir),0.0), f0);

			vec3 numerator = ndf * g * f;
			float denominator = 4.0 * max(dot(normal, viewDir),0.0) * max(dot(normal, lightDir),0.0);
			vec3 specular = numerator / max(denominator, 0.0001);

			vec3 ks = f;
			vec3 kd = vec3(1.0)- ks;
			kd *= 1.0-metalic;

			float ndotl = max(dot(normal, lightDir), 0.0);

			//complete the equation, specular + diffuse
			Lo += (kd * albedo / PI + specular) * radiance * ndotl;
		}

		vec3 ambient = vec3(0.03) * albedo;

		vec3 color = ambient + Lo;

		//gamma, linear and hdr correction
//		color = color / (color + vec3(1.0));
//		color = pow(color, vec3(1.0/2.2)); 
		vec4 outColor = vec4(color, 1.0f);

		return outColor;
}

void addObject(vec3 P, float R, int texId){//adds object to array
	
	objects[id].pos = P;
	objects[id].radius = R;
	objects[id].texId = texId;

	id++;
}

void addLight(vec3 pos, vec4 color){//adds light to array
	light[lid].pos = pos;
	light[lid].color = color;

	lid++;
}

vec4 Tracer()
{
	Ray ray = genRay(vertexPos);
	intersectResult result;
	int reflectCount = 5;
	vec4 color = vec4(0.0,0.0,0.0,1.0f);
	float previousShinyness = 1.0f;

	for(int i = 0; i < 1; i++)
	{		
		result.hit = false;
		
		result = Intersect(ray);//test for intersection
		if(result.hit)
		{
			//if hit an object shade it
			color = shade(result, ray) * previousShinyness;
			//if object is very shiny, gen new ray and redo intersection;
			if(result.shinyness > 0.9f )
			{
				previousShinyness = result.shinyness;
				//generate a new ray with reflection ray if the object is reflective
				ray = reflectionRay(ray, result);
				//restart loop as an itterative recursion approach
				i--;
				reflectCount--;
			}
		}else{
		//if it doesnt return cubemap
			color += texture(cubemap, ray.direction) * previousShinyness;
		}
		if(reflectCount <= 0)//if more than 10 reflections then stop
		{
			//if ran out of reflections return cube map
			color += texture(cubemap, ray.direction) * previousShinyness;
			break;
		}
	}
	

	//loop reflection
	return color;
};



void main(){

//set up scene
	addObject(vec3(-0.30,0.15, -1.0),0.06f, 0);
	addObject(vec3(-0.15,0.15, -1.0),0.06f, 0);
	addObject(vec3(0.0,0.15, -1.0),0.06f, 0);
	addObject(vec3(0.15,0.15, -1.0),0.06f, 0);
	addObject(vec3(0.30,0.15, -1.0),0.06f, 0);	

	addObject(vec3(-0.30,0.0, -1.0),0.06f, 0);
	addObject(vec3(-0.15,0.0, -1.0),0.06f, 0);
	addObject(vec3(0.0,0.0, -1.0),0.06f, 0);
	addObject(vec3(0.15,0.0, -1.0),0.06f, 0);
	addObject(vec3(0.30,00.0, -1.0),0.06f, 0);		

	addObject(vec3(-0.30,-0.15, -1.0),0.06f, 0);
	addObject(vec3(-0.15,-0.15, -1.0),0.06f, 0);
	addObject(vec3(0.0,-0.15, -1.0),0.06f, 0);
	addObject(vec3(0.15,-0.15, -1.0),0.06f, 0);
	addObject(vec3(0.30,-0.15, -1.0),0.06f, 0);	


	
	//set up lights

	addLight( vec3(-1.0,0.0,1.0),vec4(vec3(10.0),1.0));

	//moving light
	addLight( light_pos ,vec4(vec3(light_brightness),1.0));

	//return color is result of tracer
	fColor = Tracer();

	return;
}