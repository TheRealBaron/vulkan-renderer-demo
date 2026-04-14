#include "animations/animation.hpp"
#include "glm/gtc/matrix_transform.hpp"

#define max glm::max<float>
#define min glm::min<float>

const glm::vec3 p0(4.f, 2.f, -3.f);
const glm::vec3 p1(-8.f, 1.f, -1.f);
const glm::vec3 p2(0.f, 0.f, 0.f);

float ex(float x) {
    return x * glm::exp(x);
}

float angle(float x) {
    return 0.5f * x; // - 0.5f * glm::exp(-4.f * (x + 0.25f)) + glm::exp(3.f * (x - 13.5f));
}

float smoothMin(float a, float b, float k) {
    k *= 6.f;
    float h = max(k - abs(a - b), 0.f) / k;
    return min(a, b) - h * h * h * k * (1.f / 6.f);
}

glm::vec3 moveBeizer(float t) {
    return (p0 * (1.f - t * t)) +
        (p1 * (2.f * (1.f - t) * t)) +
        (p2 * t * t);
}

glm::vec3 moveUpDown(float t) {
    return glm::vec3(0.f, glm::sin(t) * glm::sin(t), 0.f);
}

glm::vec3 moveOutro(float t) {
    return glm::vec3(-4.f * ex(6.f * (t - 15.f)), 0.f, 0.f);
}

glm::mat4x4 animate::drammaticMovement(float time) {
    glm::vec3 pos = moveUpDown(time);
    return glm::translate(glm::mat4x4(1.f), pos);
}

glm::mat4x4 animate::drammaticRotation(float time) {
    glm::mat4 res = glm::rotate(glm::mat4(1.f), 3.1419f * 0.25f * time, glm::vec3(0.f, 1.f, 0.f));
    res = glm::rotate(res, 3.14195f * 0.5f, glm::vec3(1.f, 0.f, 0.f));
    return res;
}
