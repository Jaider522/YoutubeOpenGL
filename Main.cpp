//------- Ignore this ----------
#include<filesystem>
namespace fs = std::filesystem;
//------------------------------

#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include<stb/stb_image.h>
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>
#include "tiny_gltf_v3.h"

#include"Texture.h"
#include"shaderClass.h"
#include"VAO.h"
#include"VBO.h"
#include"EBO.h"

#include <climits>
#include <cmath>
#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include <cstring>
#include <glm/gtc/quaternion.hpp>

// Tamaño de la ventana
const unsigned int width = 800;
const unsigned int height = 800;



// Variables globales de la cámara
glm::vec3 cameraPos = glm::vec3(0.0f, 0.5f, 2.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

// Variables para controlar la rotación de la cámara
bool firstClick = true;
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = width / 2.0f;
float lastY = height / 2.0f;



// Ajusta el viewport al cambiar el tamaño de la ventana
void framebuffer_size_callback(GLFWwindow* window, int w, int h)
{
    glViewport(0, 0, w, h);
}

// Control de la cámara con mouse/touchpad
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    // Ignorar movimiento cuando el cursor está libre
    if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL)
        return;

    // Evita un salto al iniciar o recuperar el control
    if (firstClick)
    {
        lastX = xpos;
        lastY = ypos;
        firstClick = false;
    }

    // Calcula el desplazamiento del mouse
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    // Sensibilidad del mouse
    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    // Actualiza la orientación de la cámara
    yaw += xoffset;
    pitch += yoffset;

    // Limita la rotación vertical
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    // Calcula la nueva dirección de la cámara
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

int32_t loadImageCallback(
    tg3_image_result* result,
    const tg3_image_request* request,
    void* userData)
{
    if (result == nullptr ||
        request == nullptr ||
        request->data == nullptr ||
        request->data_size == 0 ||
        request->data_size > INT_MAX)
    {
        return 0;
    }

    int width = 0;
    int height = 0;
    int components = 0;

    unsigned char* pixels = stbi_load_from_memory(
        request->data,
        static_cast<int>(request->data_size),
        &width,
        &height,
        &components,
        0
    );

    if (pixels == nullptr)
        return 0;

    result->pixels = pixels;
    result->width = width;
    result->height = height;
    result->component = components;
    result->bits = 8;
    result->pixel_type = TG3_COMPONENT_TYPE_UNSIGNED_BYTE;

    return 1;
}

void freeImageCallback(
    uint8_t* pixels,
    void* userData)
{
    stbi_image_free(pixels);
}


bool loadModel(tinygltf3::Model& model, const char* filename)
{
    tinygltf3::ErrorStack errors;

    tg3_parse_options options;
    tg3_parse_options_init(&options);

    options.images_as_is = 0;
    options.image.load_image = loadImageCallback;
    options.image.free_image = freeImageCallback;

    tg3_error_code result =
        tinygltf3::parse_file(
            model,
            errors,
            filename,
            &options
        );

    if (result != TG3_OK)
    {
        std::cout << "Error al cargar el modelo: "
            << filename << std::endl;

        for (uint32_t i = 0; i < errors.count(); i++)
        {
            const tg3_error_entry* error = errors.entry(i);

            if (error != nullptr && error->message != nullptr)
                std::cout << error->message << std::endl;
        }

        return false;
    }

    std::cout << "Modelo cargado correctamente." << std::endl;
    std::cout << "Mallas: " << model->meshes_count << std::endl;
    std::cout << "Nodos: " << model->nodes_count << std::endl;
    std::cout << "Buffers: " << model->buffers_count << std::endl;
    std::cout << "Accessors: " << model->accessors_count << std::endl;
    std::cout << "\n--- DIAGNOSTICO DE IMAGENES ---" << std::endl;
    std::cout << "Cantidad de imagenes: "
        << model->images_count << std::endl;

    for (uint32_t i = 0; i < model->images_count; i++)
    {
        const tg3_image& img = model->images[i];

        std::cout << "Imagen " << i
            << " | URI: "
            << (img.uri.data ? img.uri.data : "(sin URI)")
            << " | BufferView: " << img.buffer_view
            << " | Bytes: " << img.image.count
            << " | Dimensiones: " << img.width
            << "x" << img.height
            << std::endl;
    }

    return true;
}



struct PrimitiveGPU
{
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
    GLuint texture = 0;

    GLsizei indexCount = 0;
    GLenum mode = GL_TRIANGLES;

    int material = -1;

    glm::vec4 baseColorFactor = glm::vec4(1.0f);
};

using ModelGPU = std::vector<std::vector<PrimitiveGPU>>;

// Almacena los buffers de la esfera que representa la luz.
struct LightSphere
{
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
    GLsizei indexCount = 0;
};

// Lee un atributo de tipo float desde un accessor del modelo.
bool readFloatAttribute(
    const tg3_model* model,
    int accessorIndex,
    int components,
    std::vector<float>& output)
{
    if (accessorIndex < 0 ||
        static_cast<uint32_t>(accessorIndex) >= model->accessors_count)
        return false;

    const tg3_accessor& accessor = model->accessors[accessorIndex];

    if (accessor.component_type != TG3_COMPONENT_TYPE_FLOAT ||
        accessor.buffer_view < 0 ||
        accessor.sparse.is_sparse ||
        static_cast<uint32_t>(accessor.buffer_view) >= model->buffer_views_count)
        return false;

    const tg3_buffer_view& view =
        model->buffer_views[accessor.buffer_view];

    if (view.buffer < 0 ||
        static_cast<uint32_t>(view.buffer) >= model->buffers_count)
        return false;

    const tg3_buffer& buffer = model->buffers[view.buffer];

    if (!buffer.data.data)
        return false;

    const size_t elementSize = components * sizeof(float);

    const size_t stride = view.byte_stride != 0
        ? view.byte_stride
        : elementSize;

    const uint64_t start = view.byte_offset + accessor.byte_offset;

    if (accessor.count == 0 ||
        stride < elementSize ||
        start > buffer.data.count ||
        (accessor.count - 1) >
        (buffer.data.count - start) / stride ||
        elementSize > buffer.data.count - start -
        (accessor.count - 1) * stride)
        return false;

    output.resize(static_cast<size_t>(accessor.count) * components);

    for (size_t i = 0; i < accessor.count; ++i)
    {
        const uint8_t* source =
            buffer.data.data + start + i * stride;

        std::memcpy(
            output.data() + i * components,
            source,
            elementSize
        );
    }

    return true;
}

// Lee índices de tipo unsigned byte, unsigned short o unsigned int.
bool readIndices(
    const tg3_model* model,
    int accessorIndex,
    std::vector<unsigned int>& output)
{
    if (accessorIndex < 0 ||
        static_cast<uint32_t>(accessorIndex) >= model->accessors_count)
        return false;

    const tg3_accessor& accessor = model->accessors[accessorIndex];

    if (accessor.type != TG3_TYPE_SCALAR ||
        accessor.buffer_view < 0 ||
        accessor.sparse.is_sparse ||
        static_cast<uint32_t>(accessor.buffer_view) >= model->buffer_views_count)
        return false;

    const tg3_buffer_view& view =
        model->buffer_views[accessor.buffer_view];

    if (view.buffer < 0 ||
        static_cast<uint32_t>(view.buffer) >= model->buffers_count)
        return false;

    const tg3_buffer& buffer = model->buffers[view.buffer];

    if (!buffer.data.data)
        return false;

    size_t componentSize = 0;

    switch (accessor.component_type)
    {
    case TG3_COMPONENT_TYPE_UNSIGNED_BYTE:
        componentSize = sizeof(uint8_t);
        break;

    case TG3_COMPONENT_TYPE_UNSIGNED_SHORT:
        componentSize = sizeof(uint16_t);
        break;

    case TG3_COMPONENT_TYPE_UNSIGNED_INT:
        componentSize = sizeof(uint32_t);
        break;

    default:
        return false;
    }

    const size_t stride = view.byte_stride != 0
        ? view.byte_stride
        : componentSize;

    const uint64_t start = view.byte_offset + accessor.byte_offset;

    if (accessor.count == 0 ||
        stride < componentSize ||
        start > buffer.data.count ||
        (accessor.count - 1) >
        (buffer.data.count - start) / stride ||
        componentSize > buffer.data.count - start -
        (accessor.count - 1) * stride)
        return false;

    output.resize(static_cast<size_t>(accessor.count));

    for (size_t i = 0; i < accessor.count; ++i)
    {
        const uint8_t* source =
            buffer.data.data + start + i * stride;

        if (accessor.component_type == TG3_COMPONENT_TYPE_UNSIGNED_BYTE)
        {
            uint8_t value;
            std::memcpy(&value, source, sizeof(value));
            output[i] = value;
        }
        else if (accessor.component_type == TG3_COMPONENT_TYPE_UNSIGNED_SHORT)
        {
            uint16_t value;
            std::memcpy(&value, source, sizeof(value));
            output[i] = value;
        }
        else
        {
            uint32_t value;
            std::memcpy(&value, source, sizeof(value));
            output[i] = value;
        }
    }

    return true;
}

// Crea una textura blanca para materiales sin textura.
GLuint createFallbackWhiteTexture()
{
    GLuint texture = 0;
    const unsigned char whitePixel[] = { 255, 255, 255, 255 };

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        1,
        1,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        whitePixel
    );

    glBindTexture(GL_TEXTURE_2D, 0);

    return texture;
}

// Crea una textura OpenGL leyendo directamente el archivo de imagen.
GLuint createTextureFromFile(
    const fs::path& imagePath,
    const tg3_sampler* sampler)
{
    int imageWidth = 0;
    int imageHeight = 0;
    int components = 0;

    unsigned char* pixels = stbi_load(
        imagePath.string().c_str(),
        &imageWidth,
        &imageHeight,
        &components,
        0
    );

    if (pixels == nullptr)
    {
        std::cout
            << "Error al cargar imagen: "
            << imagePath.string()
            << " | Motivo: "
            << (stbi_failure_reason()
                ? stbi_failure_reason()
                : "desconocido")
            << std::endl;

        return 0;
    }

    GLenum format = GL_RGBA;

    switch (components)
    {
    case 1:
        format = GL_RED;
        break;

    case 2:
        format = GL_RG;
        break;

    case 3:
        format = GL_RGB;
        break;

    case 4:
        format = GL_RGBA;
        break;

    default:
        stbi_image_free(pixels);
        return 0;
    }

    GLint minFilter = GL_LINEAR_MIPMAP_LINEAR;
    GLint magFilter = GL_LINEAR;
    GLint wrapS = GL_REPEAT;
    GLint wrapT = GL_REPEAT;

    // Aplica los parámetros del sampler del archivo glTF.
    if (sampler != nullptr)
    {
        if (sampler->min_filter > 0)
            minFilter = sampler->min_filter;

        if (sampler->mag_filter > 0)
            magFilter = sampler->mag_filter;

        if (sampler->wrap_s > 0)
            wrapS = sampler->wrap_s;

        if (sampler->wrap_t > 0)
            wrapT = sampler->wrap_t;
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);

    if (texture == 0)
    {
        stbi_image_free(pixels);
        return 0;
    }

    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);

    // Interpreta correctamente las imágenes de uno o dos canales.
    if (components == 1)
    {
        glTexParameteri(
            GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);

        glTexParameteri(
            GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
    }
    else if (components == 2)
    {
        glTexParameteri(
            GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);

        glTexParameteri(
            GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);

        glTexParameteri(
            GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_GREEN);
    }

    GLint previousAlignment = 4;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousAlignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        format,
        imageWidth,
        imageHeight,
        0,
        format,
        GL_UNSIGNED_BYTE,
        pixels
    );

    glGenerateMipmap(GL_TEXTURE_2D);

    glPixelStorei(GL_UNPACK_ALIGNMENT, previousAlignment);

    glBindTexture(GL_TEXTURE_2D, 0);

    std::cout
        << "Textura cargada: "
        << imagePath.filename().string()
        << " | Dimensiones: "
        << imageWidth << "x" << imageHeight
        << " | Componentes: " << components
        << std::endl;

    stbi_image_free(pixels);

    return texture;
}


// Crea las texturas declaradas en el modelo glTF.
std::vector<GLuint> createModelTextures(
    const tg3_model* model,
    const fs::path& modelDirectory)
{
    std::vector<GLuint> textures(model->textures_count, 0);

    for (uint32_t i = 0; i < model->textures_count; ++i)
    {
        const tg3_texture& texture = model->textures[i];

        if (texture.source < 0 ||
            static_cast<uint32_t>(texture.source) >= model->images_count)
        {
            std::cout
                << "Aviso: la textura " << i
                << " no tiene una imagen valida."
                << std::endl;

            continue;
        }

        const tg3_image& image = model->images[texture.source];

        // Este procedimiento carga imágenes externas mediante su URI.
        if (image.uri.data == nullptr || image.uri.data[0] == '\0')
        {
            std::cout
                << "Aviso: la imagen " << texture.source
                << " no tiene una URI externa."
                << std::endl;

            continue;
        }

        fs::path imagePath = modelDirectory / image.uri.data;

        const tg3_sampler* sampler = nullptr;

        if (texture.sampler >= 0 &&
            static_cast<uint32_t>(texture.sampler) < model->samplers_count)
        {
            sampler = &model->samplers[texture.sampler];
        }

        textures[i] = createTextureFromFile(imagePath, sampler);

        if (textures[i] == 0)
        {
            std::cout
                << "Aviso: no se pudo crear la textura "
                << i << "."
                << std::endl;
        }
    }

    return textures;
}

// Crea los buffers de OpenGL para una primitiva del modelo.
bool createPrimitiveGPU(
    const tg3_model* model,
    const tg3_primitive& primitive,
    const std::vector<GLuint>& modelTextures,
    GLuint fallbackTexture,
    PrimitiveGPU& gpu)
{
    int positionAccessor = -1;
    int uvAccessor = -1;
    int normalAccessor = -1;

    for (uint32_t i = 0; i < primitive.attributes_count; ++i)
    {
        if (tg3_str_equals_cstr(
            primitive.attributes[i].key, "POSITION"))
        {
            positionAccessor = primitive.attributes[i].value;
        }
        else if (tg3_str_equals_cstr(
            primitive.attributes[i].key, "TEXCOORD_0"))
        {
            uvAccessor = primitive.attributes[i].value;
        }
        else if (tg3_str_equals_cstr(
            primitive.attributes[i].key, "NORMAL"))
        {
            normalAccessor = primitive.attributes[i].value;
        }
    }

    std::cout
        << "Accessor de normales: "
        << normalAccessor
        << std::endl;

    // Valores por defecto.
    gpu.material = primitive.material;
    gpu.texture = fallbackTexture;
    gpu.baseColorFactor = glm::vec4(1.0f);

    // Obtiene el material y su textura de color base.
    if (primitive.material >= 0 &&
        static_cast<uint32_t>(primitive.material) < model->materials_count)
    {
        const tg3_material& material =
            model->materials[primitive.material];

        std::cout
            << "Nombre del material "
            << primitive.material
            << ": "
            << (material.name.data ? material.name.data : "(sin nombre)")
            << std::endl;

        const tg3_pbr_metallic_roughness& pbr =
            material.pbr_metallic_roughness;

        gpu.baseColorFactor = glm::vec4(
            static_cast<float>(pbr.base_color_factor[0]),
            static_cast<float>(pbr.base_color_factor[1]),
            static_cast<float>(pbr.base_color_factor[2]),
            static_cast<float>(pbr.base_color_factor[3])
        );

        const int textureIndex = pbr.base_color_texture.index;

        std::cout
            << "MATERIAL: " << primitive.material
            << " | Textura base: " << textureIndex
            << " | Accesor UV: " << uvAccessor
            << " | Factor RGBA: "
            << pbr.base_color_factor[0] << ", "
            << pbr.base_color_factor[1] << ", "
            << pbr.base_color_factor[2] << ", "
            << pbr.base_color_factor[3]
            << std::endl;

        if (uvAccessor >= 0 &&
            textureIndex >= 0 &&
            static_cast<uint32_t>(textureIndex) < model->textures_count &&
            static_cast<size_t>(textureIndex) < modelTextures.size() &&
            modelTextures[textureIndex] != 0)
        {
            gpu.texture = modelTextures[textureIndex];
        }
    }

    for (uint32_t i = 0; i < primitive.attributes_count; ++i)
    {
        if (tg3_str_equals_cstr(
            primitive.attributes[i].key, "POSITION"))
        {
            positionAccessor = primitive.attributes[i].value;
        }
        else if (tg3_str_equals_cstr(
            primitive.attributes[i].key, "TEXCOORD_0"))
        {
            uvAccessor = primitive.attributes[i].value;
        }
    }

    std::vector<float> positions;
    std::vector<float> uvs;
    std::vector<float> normals;

    if (!readFloatAttribute(model, positionAccessor, 3, positions))
        return false;

    if (!readFloatAttribute(model, uvAccessor, 2, uvs))
        return false;

    if (!readFloatAttribute(model, normalAccessor, 3, normals))
        return false;

    size_t vertexCount = positions.size() / 3;

    // Cada vértice tendrá: posición XYZ, color RGB y coordenadas UV.
    std::vector<float> vertices;
    vertices.reserve(vertexCount * 11);

    for (size_t i = 0; i < vertexCount; ++i)
    {
        // Posición
        vertices.push_back(positions[i * 3]);
        vertices.push_back(positions[i * 3 + 1]);
        vertices.push_back(positions[i * 3 + 2]);

        // Color blanco provisional
        vertices.push_back(1.0f);
        vertices.push_back(1.0f);
        vertices.push_back(1.0f);

        // Coordenadas UV
        if (uvs.size() >= (i + 1) * 2)
        {
            vertices.push_back(uvs[i * 2]);
            vertices.push_back(uvs[i * 2 + 1]);
        }
        else
        {
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
        }

        // Normales
        vertices.push_back(normals[i * 3]);
        vertices.push_back(normals[i * 3 + 1]);
        vertices.push_back(normals[i * 3 + 2]);
    }

    std::vector<unsigned int> indices;

    if (primitive.indices >= 0)
    {
        if (!readIndices(model, primitive.indices, indices))
            return false;
    }
    else
    {
        indices.resize(vertexCount);

        for (size_t i = 0; i < vertexCount; ++i)
            indices[i] = static_cast<unsigned int>(i);
    }

    if (indices.empty() ||
        indices.size() > static_cast<size_t>(INT_MAX))
    {
        return false;
    }

    switch (primitive.mode)
    {
    case TG3_MODE_POINTS:
        gpu.mode = GL_POINTS;
        break;

    case TG3_MODE_LINE:
        gpu.mode = GL_LINES;
        break;

    case TG3_MODE_LINE_LOOP:
        gpu.mode = GL_LINE_LOOP;
        break;

    case TG3_MODE_LINE_STRIP:
        gpu.mode = GL_LINE_STRIP;
        break;

    case TG3_MODE_TRIANGLE_STRIP:
        gpu.mode = GL_TRIANGLE_STRIP;
        break;

    case TG3_MODE_TRIANGLE_FAN:
        gpu.mode = GL_TRIANGLE_FAN;
        break;

    case -1:
    case TG3_MODE_TRIANGLES:
        gpu.mode = GL_TRIANGLES;
        break;

    default:
        return false;
    }

    gpu.indexCount = static_cast<GLsizei>(indices.size());

    glGenVertexArrays(1, &gpu.VAO);
    glGenBuffers(1, &gpu.VBO);
    glGenBuffers(1, &gpu.EBO);

    glBindVertexArray(gpu.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, gpu.VBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(float),
        vertices.data(),
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu.EBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int),
        indices.data(),
        GL_STATIC_DRAW
    );

    const GLsizei stride = 11 * sizeof(float);

    // Posición
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Coordenadas UV
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Normales
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);

    return true;
}

// Prepara todas las primitivas de las mallas.
ModelGPU createModelGPU(
    const tg3_model* model,
    const std::vector<GLuint>& modelTextures,
    GLuint fallbackTexture)
{
    ModelGPU gpuMeshes(model->meshes_count);

    for (uint32_t i = 0; i < model->meshes_count; ++i)
    {
        const tg3_mesh& mesh = model->meshes[i];

        gpuMeshes[i].resize(mesh.primitives_count);

        for (uint32_t j = 0; j < mesh.primitives_count; ++j)
        {
            if (!createPrimitiveGPU(
                model,
                mesh.primitives[j],
                modelTextures,
                fallbackTexture,
                gpuMeshes[i][j]))
            {
                std::cout
                    << "Aviso: no se pudo preparar la primitiva "
                    << j << " de la malla " << i << std::endl;
            }
        }
    }

    return gpuMeshes;
}

// Construye la transformación local de un nodo glTF.
glm::mat4 getNodeTransform(const tg3_node& node)
{
    if (node.has_matrix)
    {
        glm::mat4 matrix(1.0f);

        for (int column = 0; column < 4; ++column)
        {
            for (int row = 0; row < 4; ++row)
            {
                matrix[column][row] =
                    static_cast<float>(node.matrix[column * 4 + row]);
            }
        }

        return matrix;
    }

    glm::vec3 translation(
        static_cast<float>(node.translation[0]),
        static_cast<float>(node.translation[1]),
        static_cast<float>(node.translation[2])
    );

    glm::quat rotation(
        static_cast<float>(node.rotation[3]),
        static_cast<float>(node.rotation[0]),
        static_cast<float>(node.rotation[1]),
        static_cast<float>(node.rotation[2])
    );

    glm::vec3 scale(
        static_cast<float>(node.scale[0]),
        static_cast<float>(node.scale[1]),
        static_cast<float>(node.scale[2])
    );

    return glm::translate(glm::mat4(1.0f), translation)
        * glm::mat4_cast(rotation)
        * glm::scale(glm::mat4(1.0f), scale);
}

// Dibuja un nodo y sus hijos, conservando las transformaciones.
void drawNode(
    const tg3_model* model,
    const ModelGPU& gpuMeshes,
    int nodeIndex,
    const glm::mat4& parentTransform,
    GLint modelLocation,
    GLint baseColorLocation)
   
 
{
    if (nodeIndex < 0 ||
        static_cast<uint32_t>(nodeIndex) >= model->nodes_count)
        return;

    const tg3_node& node = model->nodes[nodeIndex];

    glm::mat4 transform =
        parentTransform * getNodeTransform(node);

    if (node.mesh >= 0 &&
        static_cast<uint32_t>(node.mesh) < model->meshes_count)
    {
        const auto& primitives = gpuMeshes[node.mesh];

        for (const PrimitiveGPU& primitive : primitives)
        {
            if (primitive.VAO == 0 || primitive.indexCount == 0)
                continue;

            glUniformMatrix4fv(
                modelLocation, 1, GL_FALSE,
                glm::value_ptr(transform));

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, primitive.texture);

            glUniform4fv(
                baseColorLocation,
                1,
                glm::value_ptr(primitive.baseColorFactor)
            );

            glBindVertexArray(primitive.VAO);

            glDrawElements(
                primitive.mode,
                primitive.indexCount,
                GL_UNSIGNED_INT,
                nullptr
            );
        }
    }

    for (uint32_t i = 0; i < node.children_count; ++i)
    {
        drawNode(
            model,
            gpuMeshes,
            node.children[i],
            transform,
            modelLocation,
            baseColorLocation
        );
    }
}

// Dibuja los nodos raíz de la escena seleccionada.
void drawModel(
    const tg3_model* model,
    const ModelGPU& gpuMeshes,
    GLint modelLocation,
    GLint baseColorLocation,
    const glm::mat4& modelTransform)
{
    if (model->scenes_count == 0)
        return;

    int sceneIndex = model->default_scene;

    if (sceneIndex < 0 ||
        static_cast<uint32_t>(sceneIndex) >= model->scenes_count)
    {
        sceneIndex = 0;
    }

    const tg3_scene& scene = model->scenes[sceneIndex];
    glm::mat4 rootTransform = modelTransform;

    for (uint32_t i = 0; i < scene.nodes_count; ++i)
    {
        drawNode(
            model,
            gpuMeshes,
            scene.nodes[i],
            rootTransform,
            modelLocation,
            baseColorLocation
        );
    }

    glBindVertexArray(0);
}

// Libera los buffers creados para el modelo.
void deleteModelGPU(ModelGPU& gpuMeshes)
{
    for (auto& mesh : gpuMeshes)
    {
        for (PrimitiveGPU& primitive : mesh)
        {
            if (primitive.EBO)
                glDeleteBuffers(1, &primitive.EBO);

            if (primitive.VBO)
                glDeleteBuffers(1, &primitive.VBO);

            if (primitive.VAO)
                glDeleteVertexArrays(1, &primitive.VAO);
        }
    }
}

void drawAirplaneAxes(
    Shader& shader,
    glm::mat4 airplaneTransform,
    VAO& VAO1
) {
    // Longitud de los ejes
    float length = 1.0f;

    // Eje X: rojo
    // Eje Y: verde
    // Eje Z: azul

    std::vector<float> vertices = {
        // X
        0.0f, 0.0f, 0.0f,
        length, 0.0f, 0.0f,

        // Y
        0.0f, 0.0f, 0.0f,
        0.0f, length, 0.0f,

        // Z
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, length
    };

    // Aquí necesitaremos un shader de líneas
}


// Genera la geometría de una esfera UV para representar la luz.
bool createLightSphere(LightSphere& sphere)
{
    const unsigned int sectors = 24;
    const unsigned int stacks = 16;

    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    const float PI = 3.14159265359f;

    for (unsigned int i = 0; i <= stacks; ++i)
    {
        float stackAngle = PI / 2.0f - i * PI / stacks;
        float xy = cos(stackAngle);
        float z = sin(stackAngle);

        for (unsigned int j = 0; j <= sectors; ++j)
        {
            float sectorAngle = j * 2.0f * PI / sectors;

            float x = xy * cos(sectorAngle);
            float y = xy * sin(sectorAngle);

            // Posición
            vertices.push_back(x);
            vertices.push_back(z);
            vertices.push_back(y);

            // Color blanco
            vertices.push_back(1.0f);
            vertices.push_back(1.0f);
            vertices.push_back(1.0f);

            // Coordenadas UV
            vertices.push_back(
                static_cast<float>(j) / sectors);

            vertices.push_back(
                static_cast<float>(i) / stacks);

            // Normales
            vertices.push_back(x);
            vertices.push_back(z);
            vertices.push_back(y);
        }
    }

    for (unsigned int i = 0; i < stacks; ++i)
    {
        unsigned int k1 = i * (sectors + 1);
        unsigned int k2 = k1 + sectors + 1;

        for (unsigned int j = 0; j < sectors; ++j)
        {
            if (i != 0)
            {
                indices.push_back(k1 + j);
                indices.push_back(k2 + j);
                indices.push_back(k1 + j + 1);
            }

            if (i != stacks - 1)
            {
                indices.push_back(k1 + j + 1);
                indices.push_back(k2 + j);
                indices.push_back(k2 + j + 1);
            }
        }
    }

    sphere.indexCount = static_cast<GLsizei>(indices.size());

    glGenVertexArrays(1, &sphere.VAO);
    glGenBuffers(1, &sphere.VBO);
    glGenBuffers(1, &sphere.EBO);

    glBindVertexArray(sphere.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, sphere.VBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(float),
        vertices.data(),
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere.EBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int),
        indices.data(),
        GL_STATIC_DRAW
    );

    const GLsizei stride = 11 * sizeof(float);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);

    return true;
}

//==========BEZIER==========

// Calcula un punto de una curva de Bézier cúbica
glm::vec3 bezier(
    float t,
    glm::vec3 P0,
    glm::vec3 P1,
    glm::vec3 P2,
    glm::vec3 P3
)
{
    float u = 1.0f - t;

    return
        u * u * u * P0 +
        3.0f * u * u * t * P1 +
        3.0f * u * t * t * P2 +
        t * t * t * P3;
}

// CREAR VÉRTICES DE LA CURVA

std::vector<glm::vec3> generateBezierCurve(
    glm::vec3 P0,
    glm::vec3 P1,
    glm::vec3 P2,
    glm::vec3 P3,
    int segments
)
{
    std::vector<glm::vec3> vertices;

    for (int i = 0; i <= segments; i++)
    {
        float t = (float)i / segments;

        vertices.push_back(bezier(t, P0, P1, P2, P3));
    }

    return vertices;
}

// Obtiene la posición del avión sobre la circunferencia.
glm::vec3 getCircleBezierPoint(
    float t,
    glm::vec3 center,
    float radius)
{
    const float PI = 3.14159265359f;
    const float K = 0.55228475f;

    // Determinar en cuál de los cuatro segmentos está el avión.
    float scaledT = t * 4.0f;
    int quarter = static_cast<int>(scaledT);

    if (quarter >= 4)
        quarter = 3;

    float localT = scaledT - quarter;

    float angle = quarter * (PI / 2.0f);
    float nextAngle = angle + PI / 2.0f;

    glm::vec3 P0(
        center.x + radius * cos(angle),
        center.y,
        center.z + radius * sin(angle)
    );

    glm::vec3 P3(
        center.x + radius * cos(nextAngle),
        center.y,
        center.z + radius * sin(nextAngle)
    );

    glm::vec3 tangentStart(
        -sin(angle), 0.0f, cos(angle)
    );

    glm::vec3 tangentEnd(
        -sin(nextAngle), 0.0f, cos(nextAngle)
    );

    glm::vec3 P1 = P0 + K * radius * tangentStart;
    glm::vec3 P2 = P3 - K * radius * tangentEnd;

    return bezier(localT, P0, P1, P2, P3);
}


// Genera una circunferencia horizontal mediante 4 curvas de Bézier.
std::vector<glm::vec3> generateCircleBezier(
    glm::vec3 center,
    float radius,
    int segmentsPerQuarter)
{
    std::vector<glm::vec3> vertices;

    const float PI = 3.14159265359f;
    const float K = 0.55228475f;

    for (int quarter = 0; quarter < 4; quarter++)
    {
        float angle = quarter * (PI / 2.0f);
        float nextAngle = angle + PI / 2.0f;

        glm::vec3 P0(
            center.x + radius * cos(angle),
            center.y,
            center.z + radius * sin(angle)
        );

        glm::vec3 P3(
            center.x + radius * cos(nextAngle),
            center.y,
            center.z + radius * sin(nextAngle)
        );

        glm::vec3 tangentStart(
            -sin(angle), 0.0f, cos(angle)
        );

        glm::vec3 tangentEnd(
            -sin(nextAngle), 0.0f, cos(nextAngle)
        );

        glm::vec3 P1 = P0 + K * radius * tangentStart;
        glm::vec3 P2 = P3 - K * radius * tangentEnd;

        for (int i = 0; i < segmentsPerQuarter; i++)
        {
            float t = (float)i / segmentsPerQuarter;

            vertices.push_back(bezier(t, P0, P1, P2, P3));
        }
    }

    vertices.push_back(vertices.front());

    return vertices;
}



int main()
{
    // Inicializar GLFW y configurar OpenGL 3.3
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Instanciar el objeto donde se guardará toda la información del modelo 3D
    tinygltf3::Model gltfModel;

    // Modelo del avión de papel.
    tinygltf3::Model airplaneModel;

    // Construir la ruta al archivo (ajusta esto si el nombre de las carpetas varía)
    std::string modelPath = (fs::current_path() / "Modelo" / "living_room_interior_free" / "scene.gltf").string();
    std::cout << "Ruta del modelo: "
        << modelPath << std::endl;

    // Construir la ruta del avión.
    std::string airplanePath =
        (fs::current_path() / "Modelo" / "paper_airplane" / "scene.gltf").string();

    // Cargar el avión.
    if (!loadModel(airplaneModel, airplanePath.c_str()))
    {
        return -1;
    }

    std::cout << "Existe el archivo: "
        << fs::exists(modelPath) << std::endl;

    std::cout << "Directorio actual: "
        << fs::current_path().string() << std::endl;

    // Intentar cargar
    if (!loadModel(gltfModel, modelPath.c_str())) {
        return -1; // Cierra el programa si no encuentra el modelo
    }


    fs::path modelDirectory = fs::path(modelPath).parent_path();

    for (uint32_t i = 0; i < gltfModel->images_count; i++)
    {
        const tg3_image& img = gltfModel->images[i];

        if (img.uri.data == nullptr)
            continue;

        fs::path imagePath = modelDirectory / img.uri.data;

        std::cout << "Textura " << i << ": "
            << imagePath.string()
            << " | Existe: " << fs::exists(imagePath)
            << std::endl;
    }


    // Crear la ventana
    GLFWwindow* window = glfwCreateWindow(width, height, "YoutubeOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Registrar funciones callback
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);

    // Iniciar con el cursor atrapado en modo FPS
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Inicializar GLAD y configurar el viewport
    gladLoadGL();
    glViewport(0, 0, width, height);

    // Cargar los shaders.
    Shader shaderProgram("default.vert", "default.frag");
    Shader lightShader("light.vert", "light.frag");
    Shader bezierShader("bezier.vert", "bezier.frag");

    std::vector<GLuint> modelTextures =
        createModelTextures(gltfModel.get(), modelDirectory);

    GLuint fallbackTexture = createFallbackWhiteTexture();

    // Directorio donde se encuentran los archivos del avión.
    fs::path airplaneDirectory = fs::path(airplanePath).parent_path();

    // Cargar las texturas del avión.
    std::vector<GLuint> airplaneTextures =
        createModelTextures(airplaneModel.get(), airplaneDirectory);

    // Preparar las primitivas utilizando sus materiales y texturas.
    ModelGPU gpuMeshes = createModelGPU(
        gltfModel.get(),
        modelTextures,
        fallbackTexture
    );

    // Preparar la geometría del avión.
    ModelGPU airplaneGpuMeshes = createModelGPU(
        airplaneModel.get(),
        airplaneTextures,
        fallbackTexture
    );

    // Obtener las ubicaciones de los uniformes.
    GLint modelLoc =
        glGetUniformLocation(shaderProgram.ID, "model");

    GLint viewLoc =
        glGetUniformLocation(shaderProgram.ID, "view");

    GLint projLoc =
        glGetUniformLocation(shaderProgram.ID, "proj");

    GLint texLoc =
        glGetUniformLocation(shaderProgram.ID, "tex0");

    GLint baseColorLoc =
        glGetUniformLocation(shaderProgram.ID, "baseColorFactor");

    GLuint lightPosLoc = glGetUniformLocation(shaderProgram.ID, "lightPos");
    GLuint lightColorLoc = glGetUniformLocation(shaderProgram.ID, "lightColor");
    GLuint viewPosLoc = glGetUniformLocation(shaderProgram.ID, "viewPos");

    // Uniformes del shader que representa la luz.
    GLuint lightModelLoc = glGetUniformLocation(lightShader.ID, "model");
    GLuint lightViewLoc = glGetUniformLocation(lightShader.ID, "view");
    GLuint lightProjLoc = glGetUniformLocation(lightShader.ID, "proj");
    GLuint lightColorVisualLoc = glGetUniformLocation(lightShader.ID, "lightColor");

    GLuint ambientStrengthLoc = glGetUniformLocation(shaderProgram.ID, "ambientStrength");
    GLuint specularStrengthLoc = glGetUniformLocation(shaderProgram.ID, "specularStrength");
    GLuint shininessLoc = glGetUniformLocation(shaderProgram.ID, "shininess");

    // La textura del shader utilizará la unidad GL_TEXTURE0.
    shaderProgram.Activate();
    glUniform1i(texLoc, 0);



    // Activar prueba de profundidad
    glEnable(GL_DEPTH_TEST);

    // Variables para controlar el tiempo entre frames
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    // Posición y color de la luz
    glm::vec3 lightPos(0.0f, 3.0f, 0.0f);
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
    float lightIntensity = 1.0f;
    float intensitySpeed = 1.0f;

    // Posición independiente del avión
    glm::vec3 airplanePos(1.0f, 3.0f, 1.0f);

    //============BEZIER============

    // Progreso del avión sobre la curva
    float t = 0.0f;

    // Velocidad de recorrido
    float airplaneSpeed = 0.2f;

    // Puntos de control
    // Centro y dimensiones de la circunferencia
    glm::vec3 circleCenter(0.0f, 3.5f, 0.0f);
    float circleRadius = 9.0f;

    // Generar la circunferencia mediante Bézier
    std::vector<glm::vec3> curveVertices =
        generateCircleBezier(circleCenter, circleRadius, 50);


    // Crear VAO y VBO
    VAO curveVAO;
    curveVAO.Bind();

    VBO curveVBO(
        (GLfloat*)curveVertices.data(),
        curveVertices.size() * sizeof(glm::vec3)
    );

    curveVAO.LinkAttrib(
        curveVBO,
        0,
        3,
        GL_FLOAT,
        sizeof(glm::vec3),
        (void*)0
    );

    curveVAO.Unbind();
    curveVBO.Unbind();

    //============BEZIER============

    // Intensidades de iluminación
    float ambientStrength = 0.25f;
    float specularStrength = 0.5f;
    float shininess = 32.0f;

    LightSphere lightSphere;

    if (!createLightSphere(lightSphere))
    {
        std::cout << "Error al crear la esfera de luz." << std::endl;
        return -1;
    }

    

    // Bucle principal
    while (!glfwWindowShouldClose(window))
    {
        // Calcular DeltaTime
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Limpiar pantalla y buffer de profundidad
        glClearColor(0.09f, 0.09f, 0.09f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Activar el shader
        shaderProgram.Activate();

        // Enviar la intensidad actual de la luz
        glUniform1f(
            glGetUniformLocation(shaderProgram.ID, "lightIntensity"),
            lightIntensity
        );

        // Control del cursor
        // ESC libera el cursor para interactuar con la ventana
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }

        // Clic izquierdo vuelve al modo FPS
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            firstClick = true;
        }



        // Movimiento de la cámara
        float cameraSpeed = 2.5f * deltaTime;

        // Movimiento de la fuente de luz
        float lightSpeed = 2.0f * deltaTime;
        // Aumentar intensidad (+)
        if (glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS ||
            (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS &&
                glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS))
        {
            lightIntensity += intensitySpeed * deltaTime;
        }

        // Disminuir intensidad (-)
        if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS)
        {
            lightIntensity -= intensitySpeed * deltaTime;
        }

        // Limitar intensidad
        lightIntensity = glm::clamp(lightIntensity, 0.0f, 5.0f);

        // Movimiento horizontal (X)
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            lightPos.x -= lightSpeed;

        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            lightPos.x += lightSpeed;

        // Movimiento de profundidad (Z)
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            lightPos.z -= lightSpeed;

        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            lightPos.z += lightSpeed;

        // Movimiento vertical (Y)
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            lightPos.y += lightSpeed;

        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            lightPos.y -= lightSpeed;

        // Aumentar la velocidad mientras se mantenga Shift
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
        {
            cameraSpeed *= 9.0f;
        }

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            cameraPos += cameraSpeed * cameraFront;

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            cameraPos -= cameraSpeed * cameraFront;

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;


        // Obtener el tamaño actual de la ventana
        int crntWidth, crntHeight;
        glfwGetWindowSize(window, &crntWidth, &crntHeight);

        if (crntHeight == 0)
            crntHeight = 1;

        // Inicializar matrices
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 proj = glm::mat4(1.0f);


        // Configurar la cámara
        view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        // Crear la proyección en perspectiva
        proj = glm::perspective(
            glm::radians(45.0f),
            (float)crntWidth / crntHeight,
            0.1f,
            100.0f
        );

        // Enviar las matrices de cámara al shader
        glUniformMatrix4fv(
            viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        glUniformMatrix4fv(
            projLoc, 1, GL_FALSE, glm::value_ptr(proj));

        // Propiedades de la luz
        glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
        glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

        // Posición actual de la cámara
        glUniform3fv(viewPosLoc, 1, glm::value_ptr(cameraPos));

        // Intensidades
        glUniform1f(ambientStrengthLoc, ambientStrength);
        glUniform1f(specularStrengthLoc, specularStrength);
        glUniform1f(shininessLoc, shininess);

        // Dibujar el modelo de la sala
        drawModel(
            gltfModel.get(),
            gpuMeshes,
            modelLoc,
            baseColorLoc,
            glm::mat4(1.0f)
        );


        // Avanzar por la curva
        t += airplaneSpeed * deltaTime;

        if (t >= 1.0f)
            t -= 1.0f;

        // Posición exacta del avión sobre la curva
        airplanePos = getCircleBezierPoint(
            t,
            circleCenter,
            circleRadius
        );

        // Obtener un punto adelantado
        float lookAhead = 0.001f;
        float nextT = t + lookAhead;

        if (nextT >= 1.0f)
            nextT -= 1.0f;

        glm::vec3 nextPos = getCircleBezierPoint(
            nextT,
            circleCenter,
            circleRadius
        );

        // Dirección de movimiento
        glm::vec3 direction = glm::normalize(nextPos - airplanePos);

        // Orientación
        glm::mat4 orientation = glm::inverse(
            glm::lookAt(
                glm::vec3(0.0f),
                direction,
                glm::vec3(0.0f, 1.0f, 0.0f)
            )
        );

        // Transformación del avión
        glm::mat4 airplaneTransform(1.0f);

        airplaneTransform = glm::translate(
            airplaneTransform,
            airplanePos
        );

        airplaneTransform *= orientation;

        // Dibujar los ejes con la misma orientación del avión
        glm::mat4 axesTransform = airplaneTransform;

        // Escala del avión
        airplaneTransform = glm::scale(
            airplaneTransform,
            glm::vec3(3.0f)
        );

        // Dibujar avión
        drawModel(
            airplaneModel.get(),
            airplaneGpuMeshes,
            modelLoc,
            baseColorLoc,
            airplaneTransform
        );

        // Dibujar la curva de Bézier
        bezierShader.Activate();

        glUniformMatrix4fv(
            glGetUniformLocation(bezierShader.ID, "view"),
            1,
            GL_FALSE,
            glm::value_ptr(view)
        );

        glUniformMatrix4fv(
            glGetUniformLocation(bezierShader.ID, "proj"),
            1,
            GL_FALSE,
            glm::value_ptr(proj)
        );

        curveVAO.Bind();

        glLineWidth(3.0f);

        glDrawArrays(
            GL_LINE_STRIP,
            0,
            (GLsizei)curveVertices.size()
        );

        curveVAO.Unbind();

        
        // Dibujar la esfera que representa la fuente de luz.
        lightShader.Activate();

        glm::mat4 lightModel = glm::mat4(1.0f);
        lightModel = glm::translate(lightModel, lightPos);
        lightModel = glm::scale(lightModel, glm::vec3(0.12f));

        glUniformMatrix4fv(lightModelLoc, 1, GL_FALSE, glm::value_ptr(lightModel));
        glUniformMatrix4fv(lightViewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(lightProjLoc, 1, GL_FALSE, glm::value_ptr(proj));
        glUniform3fv(lightColorVisualLoc, 1, glm::value_ptr(lightColor));

        glBindVertexArray(lightSphere.VAO);
        glDrawElements(GL_TRIANGLES, lightSphere.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Mostrar el frame y procesar eventos
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Liberar los recursos del modelo.
    deleteModelGPU(gpuMeshes);
    deleteModelGPU(airplaneGpuMeshes);

    for (GLuint texture : airplaneTextures)
    {
        if (texture != 0)
            glDeleteTextures(1, &texture);
    }

    // Liberar los recursos de la esfera de luz.
    if (lightSphere.EBO)
        glDeleteBuffers(1, &lightSphere.EBO);

    if (lightSphere.VBO)
        glDeleteBuffers(1, &lightSphere.VBO);

    if (lightSphere.VAO)
        glDeleteVertexArrays(1, &lightSphere.VAO);

    for (GLuint texture : modelTextures)
    {
        if (texture != 0)
            glDeleteTextures(1, &texture);
    }

    if (fallbackTexture != 0)
        glDeleteTextures(1, &fallbackTexture);

    shaderProgram.Delete();
    lightShader.Delete();
    bezierShader.Delete();

    curveVAO.Delete();
    curveVBO.Delete();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}