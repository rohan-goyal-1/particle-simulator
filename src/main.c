#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "vendors/glad/glad.h"
#include "vendors/GLFW/glfw3.h"

#define GL_SILENCE_DEPRECATION

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
#define NUM_CIRCLE_SEGMENTS 100
#define MAX_OBJECTS 100

float current_radius = 0.1f;

const float gravity = -0.0005f;
const float bounce_restitution = 0.75f;

GLuint vertex_shader;
GLuint fragment_shader;
GLuint outline_fragment_shader;

int amount_balls = 0;

struct Ball {
    float radius;
    float x_pos, y_pos;
    float x_vel, y_vel;
    float mass;
} balls[MAX_OBJECTS];

double mouse_x = 0.0, mouse_y = 0.0;

void build_circle (GLfloat* vertices, float x, float y, float radius) {
    float twicePi = 2.0f * M_PI;
    vertices[0] = x;
    vertices[1] = y;
    vertices[2] = 0.0f;
    for (int i = 1; i <= NUM_CIRCLE_SEGMENTS; i++) {
        vertices[i * 3] = x + (radius * cos(i * twicePi / NUM_CIRCLE_SEGMENTS));
        vertices[i * 3 + 1] = y + (radius * sin(i * twicePi / NUM_CIRCLE_SEGMENTS));
        vertices[i * 3 + 2] = 0.0f;
    }
    vertices[(NUM_CIRCLE_SEGMENTS + 1) * 3] = vertices[3];
    vertices[(NUM_CIRCLE_SEGMENTS + 1) * 3 + 1] = vertices[4];
    vertices[(NUM_CIRCLE_SEGMENTS + 1) * 3 + 2] = vertices[5];
}

void scroll_callback (GLFWwindow* window, double xoffset, double yoffset) {
    current_radius += yoffset * 0.05f;
    if (current_radius < 0.01f) current_radius = 0.01f;
    if (current_radius > 0.25f) current_radius = 0.25f;
}

void add_ball (float x_pos, float y_pos, float radius) {
    struct Ball new_ball = {radius, x_pos, y_pos, 0.0f, 0.0f, M_PI * radius * radius * radius};
    balls[amount_balls] = new_ball;
    amount_balls++;
}

void mouse_button_callback (GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double x_pos, y_pos;
        glfwGetCursorPos(window, &x_pos, &y_pos);

        x_pos = (x_pos / WINDOW_WIDTH) * 2.0 - 1.0;
        y_pos = 1.0 - (y_pos / WINDOW_HEIGHT) * 2.0;

        add_ball(x_pos, y_pos, current_radius);
    }
}

void cursor_position_callback (GLFWwindow* window, double xpos, double ypos) {
    mouse_x = xpos;
    mouse_y = ypos;
}

void update_ball (struct Ball* ball) {
    ball->y_vel += gravity;
    ball->x_pos += ball->x_vel;
    ball->y_pos += ball->y_vel;
}

void apply_constraints (struct Ball* ball) {
    float bottom_limit = -1.0f + ball->radius;
    if (ball->y_pos < bottom_limit) {
        ball->y_pos = bottom_limit;
        ball->y_vel = -ball->y_vel * bounce_restitution;
    }

    float top_limit = 1.0f - ball->radius;
    if (ball->y_pos > top_limit) {
        ball->y_pos = top_limit;
        ball->y_vel = -ball->y_vel * bounce_restitution;
    }

    float left_limit = -1.0f + ball->radius;
    if (ball->x_pos < bottom_limit) {
        ball->x_pos = bottom_limit;
        ball->x_vel = -ball->x_vel * bounce_restitution;
    }

    float right_limit = 1.0f - ball->radius;
    if (ball->x_pos > top_limit) {
        ball->x_pos = top_limit;
        ball->x_vel = -ball->x_vel * bounce_restitution;
    }
}
void handle_collisions () {
    for (int i = 0; i < amount_balls; i++) {
        for (int j = i + 1; j < amount_balls; j++) {
            float dx = balls[j].x_pos - balls[i].x_pos;
            float dy = balls[j].y_pos - balls[i].y_pos;
            float distance_squared = dx * dx + dy * dy;
            float radius_sum = balls[i].radius + balls[j].radius;

            if (distance_squared <= (radius_sum * radius_sum)) {
                float distance = sqrt(distance_squared);

                if (distance == 0.0f) {
                    distance = 0.1f;
                }

                float overlap = radius_sum - distance;
                float nx = dx / distance;
                float ny = dy / distance;
                float displacement_i = overlap * (balls[j].radius / radius_sum);
                float displacement_j = overlap * (balls[i].radius / radius_sum);
                balls[i].x_pos -= nx * displacement_i;
                balls[i].y_pos -= ny * displacement_i;
                balls[j].x_pos += nx * displacement_j;
                balls[j].y_pos += ny * displacement_j;

                float nx_total = nx * (balls[i].x_vel - balls[j].x_vel) + ny * (balls[i].y_vel - balls[j].y_vel);
                float p = 2.0f * nx_total / (balls[i].mass + balls[j].mass);

                balls[i].x_vel -= p * balls[j].mass * nx;
                balls[i].y_vel -= p * balls[j].mass * ny;
                balls[j].x_vel += p * balls[i].mass * nx;
                balls[j].y_vel += p * balls[i].mass * ny;
            }
        }
    }
}

void draw_circle (GLfloat* vertices, GLuint VBO, GLuint VAO, GLuint shader_program) {
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, (NUM_CIRCLE_SEGMENTS + 2) * 3 * sizeof(GLfloat), vertices, GL_DYNAMIC_DRAW);
    glUseProgram(shader_program);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, NUM_CIRCLE_SEGMENTS + 2);
}

void draw_outline (GLuint VBO, GLuint VAO, GLuint outline_shader_program) {
    GLfloat outline_vertices[(NUM_CIRCLE_SEGMENTS + 2) * 3];
    double x_pos = (mouse_x / WINDOW_WIDTH) * 2.0 - 1.0;
    double y_pos = 1.0 - (mouse_y / WINDOW_HEIGHT) * 2.0;
    build_circle(outline_vertices, x_pos, y_pos, current_radius);
    draw_circle(outline_vertices, VBO, VAO, outline_shader_program);
}

char* read_file (const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file %s\n", filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* file_contents = (char*)malloc(file_size + 1);
    if (file_contents == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        fclose(file);
        exit(1);
    }

    size_t read_size = fread(file_contents, 1, file_size, file);
    if (read_size != file_size) {
        fprintf(stderr, "Error reading file %s\n", filename);
        fclose(file);
        free(file_contents);
        exit(1);
    }

    file_contents[file_size] = '\0';

    fclose(file);
    return file_contents;
}

GLuint compile_shader (GLenum shader_type, const char* shader_source) {
    GLuint shader = glCreateShader(shader_type);
    glShaderSource(shader, 1, &shader_source, NULL);
    glCompileShader(shader);

    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        fprintf(stderr, "Shader Compilation Failed: %s\n", infoLog);
        exit(1);
    }

    return shader;
}

GLuint link_program (GLuint vertex_shader, GLuint fragment_shader) {
    GLuint shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    GLint success;
    GLchar infoLog[512];
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader_program, 512, NULL, infoLog);
        fprintf(stderr, "Shader Program Linking Failed: %s\n", infoLog);
        exit(1);
    }

    return shader_program;
}

GLuint create_shader (const char* frag_path, const char* vert_path) {
    char* frag_source = read_file(frag_path);
    char* vert_source = read_file(vert_path);

    vertex_shader = compile_shader(GL_VERTEX_SHADER, vert_source);
    fragment_shader = compile_shader(GL_FRAGMENT_SHADER, frag_source);

    GLuint shader_program = link_program(vertex_shader, fragment_shader);

    free(frag_source);
    free(vert_source);

    return shader_program;
}

int main () {
    if (!glfwInit()) {
        fprintf(stderr, "ERROR: Could not initialize GLFW.");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Particle Simulator", NULL, NULL);
    if (!window) {
        fprintf(stderr, "ERROR: Could not open window.");
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "ERROR: Failed to initialize GLAD.");
        return 1;
    }

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    GLuint shader_program = create_shader("shaders/ball_default.frag", "shaders/default.vert");
    GLuint outline_shader_program = create_shader("shaders/ball_outline.frag", "shaders/default.vert");

    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        draw_outline(VBO, VAO, outline_shader_program);

        handle_collisions();

        for (int i = 0; i < amount_balls; i++) {
            GLfloat vertices[(NUM_CIRCLE_SEGMENTS + 2) * 3];
            update_ball(&balls[i]);
            build_circle(vertices, balls[i].x_pos, balls[i].y_pos, balls[i].radius);
            draw_circle(vertices, VBO, VAO, shader_program);
        }

        for (int i = 0; i < amount_balls; i++) {
            apply_constraints(&balls[i]);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
