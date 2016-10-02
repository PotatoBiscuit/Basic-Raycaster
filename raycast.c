#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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

void store_value(Object* input_object, int type_of_field, double input_value, double* input_vector){
	//type_of_field values: 0 = width, 1 = height, 2 = radius, 3 = color, 4 = position, 5 = normal
	//if input_value or input_vector aren't used, a 0 or NULL value should be passed in
	if(input_object->kind == 0){
		if(type_of_field == 0){
			input_object->camera.width = input_value;
		}else if(type_of_field == 1){
			input_object->camera.height = input_value;
		}else{
			fprintf(stderr, "Error: Camera may only have 'width' or 'height' fields, line:%d\n", line);
			exit(1);
		}
	}else if(input_object->kind == 1){
		if(type_of_field == 2){
			input_object->sphere.radius = input_value;
		}else if(type_of_field == 3){
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

int main(int c, char** argv) {
	Object** object_array = malloc(sizeof(Object*)*130);
	int object_counter;
	object_array[129] = NULL;
	
	
	argument_checker(c, argv);
	object_counter = read_scene(argv[3], object_array);	//Parse .json scene file
	//raycast_scene() //FINISH
	
	return 0;
}