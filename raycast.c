#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

typedef struct {
  int kind; // 0 = camera, 1 = sphere, 2 = plane
  union {
    struct {
      double width;
      double height;
    } camera;
    struct {
      double color[3];
	  double position[3];
      double radius;
    } sphere;
    struct {
      double color[3];
	  double position[3];
	  double normal[3];
    } plane;
  };
} Object;

int line = 1;

// next_c() wraps the getc() function and provides error checking and line
// number maintenance
int next_c(FILE* json) {
  int c = fgetc(json);
#ifdef DEBUG
  printf("next_c: '%c'\n", c);
#endif
  if (c == '\n') {
    line += 1;
  }
  if (c == EOF) {
    fprintf(stderr, "Error: Unexpected end of file on line number %d.\n", line);
    exit(1);
  }
  return c;
}


// expect_c() checks that the next character is d.  If it is not it emits
// an error.
void expect_c(FILE* json, int d) {
  int c = next_c(json);
  if (c == d) return;
  fprintf(stderr, "Error: Expected '%c' on line %d.\n", d, line);
  exit(1);    
}


// skip_ws() skips white space in the file.
void skip_ws(FILE* json) {
  int c = next_c(json);
  while (isspace(c)) {
    c = next_c(json);
  }
  ungetc(c, json);
}


// next_string() gets the next string from the file handle and emits an error
// if a string can not be obtained.
char* next_string(FILE* json) {
  char buffer[129];
  int c = next_c(json);
  if (c != '"') {
    fprintf(stderr, "Error: Expected string on line %d.\n", line);
    exit(1);
  }  
  c = next_c(json);
  int i = 0;
  while (c != '"') {
    if (i >= 128) {
      fprintf(stderr, "Error: Strings longer than 128 characters in length are not supported.\n");
      exit(1);      
    }
    if (c == '\\') {
      fprintf(stderr, "Error: Strings with escape codes are not supported.\n");
      exit(1);      
    }
    if (c < 32 || c > 126) {
      fprintf(stderr, "Error: Strings may contain only ascii characters.\n");
      exit(1);
    }
    buffer[i] = c;
    i += 1;
    c = next_c(json);
  }
  buffer[i] = 0;
  return strdup(buffer);
}

double next_number(FILE* json) {
	double value;
	int numDigits = 0;
	numDigits = fscanf(json, "%lf", &value);
	if(numDigits == 0){
		fprintf(stderr, "Error: Expected number at line %d\n", line);
		exit(1);
	}
	return value;
}

double* next_vector(FILE* json) {
	double* v = malloc(3*sizeof(double));
	expect_c(json, '[');
	skip_ws(json);
	v[0] = next_number(json);
	skip_ws(json);
	expect_c(json, ',');
	skip_ws(json);
	v[1] = next_number(json);
	skip_ws(json);
	expect_c(json, ',');
	skip_ws(json);
	v[2] = next_number(json);
	skip_ws(json);
	expect_c(json, ']');
	return v;
}

static inline double sqr(double v) {
  return v*v;
}

static inline void normalize(double* v) {
	double len = sqrt(sqr(v[0]) + sqr(v[1]) + sqr(v[2]));
	v[0] /= len;
	v[1] /= len;
	v[2] /= len;
}

void store_value(Object* input_object, int type_of_field, double input_value, double* input_vector){
	//type_of_field values: 0 = width, 1 = height, 2 = radius, 3 = color, 4 = position, 5 = normal
	//if input_value or input_vector aren't used, a 0 or NULL value should be passed in
	if(input_object->kind == 0){
		if(type_of_field == 0){
			if(input_value <= 0){
				fprintf(stderr, "Error: Camera width must be greater than 0, line:%d\n", line);
				exit(1);
			}
			input_object->camera.width = input_value;
		}else if(type_of_field == 1){
			if(input_value <= 0){
				fprintf(stderr, "Error: Camera height must be greater than 0, line:%d\n", line);
				exit(1);
			}
			input_object->camera.height = input_value;
		}else{
			fprintf(stderr, "Error: Camera may only have 'width' or 'height' fields, line:%d\n", line);
			exit(1);
		}
	}else if(input_object->kind == 1){
		if(type_of_field == 2){
			input_object->sphere.radius = input_value;
		}else if(type_of_field == 3){
			if(input_vector[0] > 1 || input_vector[1] > 1 || input_vector[2] > 1){
				fprintf(stderr, "Error: Color values must be between 0 and 1, line:%d\n", line);
				exit(1);
			}
			if(input_vector[0] < 0 || input_vector[1] < 0 || input_vector[2] < 0){
				fprintf(stderr, "Error: Color values may not be negative, line:%d\n", line);
				exit(1);
			}
			input_object->sphere.color[0] = input_vector[0];
			input_object->sphere.color[1] = input_vector[1];
			input_object->sphere.color[2] = input_vector[2];
		}else if(type_of_field == 4){
			input_object->sphere.position[0] = input_vector[0];
			input_object->sphere.position[1] = input_vector[1];
			input_object->sphere.position[2] = input_vector[2];
		}else{
			fprintf(stderr, "Error: Spheres only have 'radius', 'color', or 'position' fields, line:%d\n", line);
			exit(1);
		}
	}else if(input_object->kind == 2){
		if(type_of_field == 3){
			if(input_vector[0] > 1 || input_vector[1] > 1 || input_vector[2] > 1){
				fprintf(stderr, "Error: Color values must be between 0 and 1, line:%d\n", line);
				exit(1);
			}
			if(input_vector[0] < 0 || input_vector[1] < 0 || input_vector[2] < 0){
				fprintf(stderr, "Error: Color values may not be negative, line:%d\n", line);
				exit(1);
			}
			input_object->plane.color[0] = input_vector[0];
			input_object->plane.color[1] = input_vector[1];
			input_object->plane.color[2] = input_vector[2];
		}else if(type_of_field == 4){
			input_object->plane.position[0] = input_vector[0];
			input_object->plane.position[1] = input_vector[1];
			input_object->plane.position[2] = input_vector[2];
		}else if(type_of_field == 5){
			input_object->plane.normal[0] = input_vector[0];
			input_object->plane.normal[1] = input_vector[1];
			input_object->plane.normal[2] = input_vector[2];
			normalize(input_object->plane.normal);
		}else{
			fprintf(stderr, "Error: Planes only have 'radius', 'color', or 'normal' fields, line:%d\n", line);
			exit(1);
		}
	}else{
		fprintf(stderr, "Error: Undefined object type, line:%d\n", line);
		exit(1);
	}
}

int read_scene(char* filename, Object** object_array) {
  int c;
  int num_objects = 0;
  int object_counter = -1;
  FILE* json = fopen(filename, "r");

  if (json == NULL) {
    fprintf(stderr, "Error: Could not open file \"%s\"\n", filename);
    exit(1);
  }
  
  skip_ws(json);
  
  // Find the beginning of the list
  expect_c(json, '[');

  skip_ws(json);

  // Find the objects
  while (1) {
    c = fgetc(json);
    if (c == ']' && num_objects != 0) {
      fprintf(stdout, "JSON parsing complete!\n");
      fclose(json);
      return object_counter;
    }
	else if(c == ']'){
		fprintf(stderr, "Error: JSON file contains no objects\n");
		fclose(json);
		exit(1);
	}
	
    if (c == '{') {
	  if(object_counter >= 128){
		  fprintf(stderr, "Error: Maximum amount of objects allowed (not including the camera) is 128, line:%d\n", line);
		  exit(1);
	  }
	  object_array[++object_counter] = malloc(sizeof(Object));
      skip_ws(json);
    
      // Parse the object
      char* key = next_string(json);
      if (strcmp(key, "type") != 0) {
	fprintf(stderr, "Error: Expected \"type\" key on line number %d.\n", line);
	exit(1);
      }

      skip_ws(json);

      expect_c(json, ':');

      skip_ws(json);

      char* value = next_string(json);

      if (strcmp(value, "camera") == 0) {
		  object_array[object_counter]->kind = 0;
      } else if (strcmp(value, "sphere") == 0) {
		  object_array[object_counter]->kind = 1;
      } else if (strcmp(value, "plane") == 0) {
		  object_array[object_counter]->kind = 2;
      } else {
	fprintf(stderr, "Error: Unknown type, \"%s\", on line number %d.\n", value, line);
	exit(1);
      }

      skip_ws(json);

      while (1) {
		// , }
		c = next_c(json);
		if (c == '}') {
		  // stop parsing this object
		  break;
		} else if (c == ',') {
		  // read another field
		  skip_ws(json);
		  char* key = next_string(json);
		  skip_ws(json);
		  expect_c(json, ':');
		  skip_ws(json);
		  if (strcmp(key, "width") == 0){
			  double value = next_number(json);
			  store_value(object_array[object_counter], 0, value, NULL);
		  }else if(strcmp(key, "height") == 0){
			  double value = next_number(json);
			  store_value(object_array[object_counter], 1, value, NULL);
		  }else if(strcmp(key, "radius") == 0) {
			  double value = next_number(json);
			  store_value(object_array[object_counter], 2, value, NULL);
		  } else if (strcmp(key, "color") == 0){
			  double* value = next_vector(json);
			  store_value(object_array[object_counter], 3, 0, value);
		  }else if(strcmp(key, "position") == 0){
			  double* value = next_vector(json);
			  store_value(object_array[object_counter], 4, 0, value);
		  }else if(strcmp(key, "normal") == 0) {
			  double* value = next_vector(json);
			  store_value(object_array[object_counter], 5, 0, value);
		  } else {
			fprintf(stderr, "Error: Unknown property, \"%s\", on line %d.\n",
				key, line);
				exit(1);
			//char* value = next_string(json);
		  }
		  skip_ws(json);
		} else {
		  fprintf(stderr, "Error: Unexpected value on line %d\n", line);
		  exit(1);
		}
      }
      skip_ws(json);
	  num_objects++;
      c = next_c(json);
      if (c == ',') {
	// noop
	skip_ws(json);
      } else if (c == ']') {
	fprintf(stdout, "JSON parsing complete!\n");
	fclose(json);
	return object_counter;
      } else {
	fprintf(stderr, "Error: Expecting ',' or ']' on line %d.\n", line);
	exit(1);
      }
    }
  }
}

void argument_checker(int c, char** argv){
	int i = 0;
	int j = 0;
	char* periodPointer;
	if(c != 5){	//Ensure that five arguments are passed in through command line
		fprintf(stderr, "Error: Incorrect amount of arguments\n");
		exit(1);
	}
	
	while(1){	//Ensure that both the width and height arguments are numbers
		if(*(argv[1] + i) == NULL && *(argv[2] + j) == NULL){
			break;
		}
		else if(*(argv[1] + i) == NULL){
			i--;
		}
		else if(*(argv[2] + j) == NULL){
			j--;
		}
		
		if(!isdigit(*(argv[1] + i)) || !isdigit(*(argv[2] + j))){
			fprintf(stderr, "Error: Width or Height field is not a number\n");
			exit(1);
		}
		i++;
		j++;
	}
	
	periodPointer = strrchr(argv[3], '.');	//Ensure that the input scene file has an extension .json
	if(periodPointer == NULL){
		fprintf(stderr, "Error: Input scene file does not have a file extension\n");
		exit(1);
	}
	if(strcmp(periodPointer, ".json") != 0){
		fprintf(stderr, "Error: Input scene file is not of type JSON\n");
		exit(1);
	}
	
	periodPointer = strrchr(argv[4], '.');	//Ensure that the output picture file has an extension .ppm
	if(periodPointer == NULL){
		fprintf(stderr, "Error: Output picture file does not have a file extension\n");
		exit(1);
	}
	if(strcmp(periodPointer, ".ppm") != 0){
		fprintf(stderr, "Error: Output picture file is not of type PPM\n");
		exit(1);
	}
}

double sphere_intersection(double* Ro, double* Rd, double* C, double radius){
	//Sphere equation is x^2 + y^2 + z^2 = r^2
	//Parameterize: (x-Cx)^2 + (y-Cy)^2 + (z-Cz)^2 - r^2 = 0
	//Substitute with ray:
	//(Rox + t*Rdx - Cx)^2 + (Roy + t*Rdy - Cy)^2 + (Roz + t*Rdz - Cz)^2 - r^2 = 0
	//Solve for t:
	//a = Rdx^2 + Rdy^2 + Rdz^2
	double a = sqr(Rd[0]) + sqr(Rd[1]) + sqr(Rd[2]);
	//b = (2RdxRox - 2RdxCx) + (2RdyRoy - 2RdyCy) + (2RdzRoz - 2RdzCz)
	double b = (2*Rd[0]*Ro[0] - 2*Rd[0]*C[0]) + (2*Rd[1]*Ro[1] - 2*Rd[1]*C[1]) + (2*Rd[2]*Ro[2] - 2*Rd[2]*C[2]);
	//c = (Rox^2 - 2RoxCx + Cx^2) + (Roy^2 - 2RoyCy + Cy^2) + (Roz^2 - 2RozCz + Cz^2) - r^2
	double c = (sqr(Ro[0]) - 2*Ro[0]*C[0] + sqr(C[0])) + (sqr(Ro[1]) - 2*Ro[1]*C[1] + sqr(C[1])) + (sqr(Ro[2]) - 2*Ro[2]*C[2] + sqr(C[2])) - sqr(radius);
	
	double t0;
	double t1;
	double det = sqr(b) - 4*a*c;
	if(det < 0) return 0;
	
	t0 = (-b - det)/(2*a);
	if(t0 > 0) return t0;
	
	t1 = (-b + det)/(2*a);
	if(t1 > 0) return t1;
	
	return 0;
}

double plane_intersection(double* Ro, double* Rd, double* C, double* N){
	//Solve for Plane Equation:
	//Nx(x - Cx) + Ny(y - Cy) + Nz(z - Cz) = 0
	//Plug in our Ray:
	//Nx(Rox + t*Rdx - Cx) + Ny(Roy + t*Rdy - Cy) + Nz(Roz + t*Rdz - Cz) = 0
	//Solve for t:
	//t = ((NxCx - NxRox) + (NyCy - NyRoy) + (NzCz - NzRoz))/(RdxNx + RdyNy + RdzNz)
	double t = ((N[0]*C[0] - N[0]*Ro[0]) + (N[1]*C[1] - N[1]*Ro[1]) + (N[2]*C[2] - N[2]*Ro[2]))/(Rd[0]*N[0] + Rd[1]*N[1] + Rd[2]*N[2]);
	if(t > 0) return t;
	return 0;
}

void raycast_scene(Object** object_array, int object_counter, double** pixel_buffer, int N, int M){
	int parse_count = 0;
	int pixel_count = 0;
	int i;
	int x;
	int y;
	double Ro[3];
	double Rd[3];
	double cx = 0;
	double cy = 0;
	double w;
	double h;
	double pixwidth;
	double pixheight;
	double t;
	
	if(object_array[parse_count]->kind != 0){
		fprintf(stderr, "Error: First object must be of type camera\n");
		exit(1);
	}
	
	while(parse_count < object_counter + 1){
		if(object_array[parse_count]->kind == 0){
			if(parse_count != 0){
				fprintf(stderr, "Error: You may only have one camera\n");
				exit(1);
			}
			w = object_array[parse_count]->camera.width;
			pixwidth = w/N;
			h = object_array[parse_count]->camera.height;
			pixheight = h/M;
			parse_count++;
			continue;
		}else if(object_array[parse_count]->kind == 1){
			for(y = 0; y < M; y += 1){
				for(x = 0; x < N; x += 1){
					Ro[0] = 0;
					Ro[1] = 0;
					Ro[2] = 0;
					Rd[0] = cx - (w/2) + pixwidth * (x + .5);
					Rd[1] = cy - (h/2) + pixheight * (y + .5);
					Rd[2] = 1;
					normalize(Rd);
					
					t = sphere_intersection(Ro, Rd, object_array[parse_count]->sphere.position,
											object_array[parse_count]->sphere.radius);
					if(t > 0 && t != INFINITY){
						pixel_buffer[(int)((N*M - 1) - (floor(pixel_count/N) + 1)*N)+ pixel_count%N][0] = object_array[parse_count]->sphere.color[0];
						pixel_buffer[(int)((N*M - 1) - (floor(pixel_count/N) + 1)*N)+ pixel_count%N][1] = object_array[parse_count]->sphere.color[1];
						pixel_buffer[(int)((N*M - 1) - (floor(pixel_count/N) + 1)*N)+ pixel_count%N][2] = object_array[parse_count]->sphere.color[2];
						pixel_count++;
					}else{
						pixel_count++;
					}
				}
			}
			pixel_count = 0;
			parse_count++;
		}else if(object_array[parse_count]->kind == 2){
			for(y = 0; y < M; y += 1){
				for(x = 0; x < N; x += 1){
					Ro[0] = 0;
					Ro[1] = 0;
					Ro[2] = 0;
					Rd[0] = cx - (w/2) + pixwidth * (x + .5);
					Rd[1] = cy - (h/2) + pixheight * (y + .5);
					Rd[2] = 1;
					normalize(Rd);
					
					t = plane_intersection(Ro, Rd, object_array[parse_count]->plane.position,
											object_array[parse_count]->plane.normal);
					if(t > 0 && t != INFINITY){
						pixel_buffer[(int)((N*M - 1) - (floor(pixel_count/N) + 1)*N)+ pixel_count%N][0] = object_array[parse_count]->plane.color[0];
						pixel_buffer[(int)((N*M - 1) - (floor(pixel_count/N) + 1)*N)+ pixel_count%N][1] = object_array[parse_count]->plane.color[1];
						pixel_buffer[(int)((N*M - 1) - (floor(pixel_count/N) + 1)*N)+ pixel_count%N][2] = object_array[parse_count]->plane.color[2];
						pixel_count++;
					}else{
						pixel_count++;
					}
				}
			}
			pixel_count = 0;
			parse_count++;
		}else{
			fprintf(stderr, "Error: Unrecognized Object\n");
			exit(1);
		}
	}
	printf("All objects have been rendered!\n");
}

void create_image(double** pixel_buffer, char* output, int width, int height){
	FILE *output_pointer = fopen(output, "wb");	/*Open the output file*/
	char buffer[width*height*3];
	int counter = 0;
	
	while(counter < width*height){
		buffer[counter*3] = (int)(255*pixel_buffer[counter][0]);
		buffer[counter*3 + 1] = (int)(255*pixel_buffer[counter][1]);
		buffer[counter*3 + 2] = (int)(255*pixel_buffer[counter][2]);
		counter++;
	}
	fprintf(output_pointer, "P6\n%d %d\n255\n", width, height);
	fwrite(buffer, sizeof(char), width*height*3, output_pointer);
	
	fprintf(stdout, "Write to %s complete!\n", output);
	fclose(output_pointer);
}

int main(int c, char** argv) {
	Object** object_array = malloc(sizeof(Object*)*130);
	int width;
	int height;
	double** pixel_buffer;
	int object_counter;
	int counter = 0;
	object_array[129] = NULL;
	
	argument_checker(c, argv);
	
	width = atoi(argv[1]);
	height = atoi(argv[2]);
	
	pixel_buffer = malloc(sizeof(double*)*(width*height + 1));
	pixel_buffer[width*height] = NULL;
	
	while(counter < width*height){
		pixel_buffer[counter] = malloc(sizeof(double)*3);
		pixel_buffer[counter][0] = 0;
		pixel_buffer[counter][1] = 0;
		pixel_buffer[counter][2] = 0;
		counter++;
	}
	
	object_counter = read_scene(argv[3], object_array);	//Parse .json scene file
	raycast_scene(object_array, object_counter, pixel_buffer, width, height);
	create_image(pixel_buffer, argv[4], width, height);
	
	return 0;
}