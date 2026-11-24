# YoutubeOpenGL

Proyecto de renderizado y visualización tridimensional desarrollado en **C++ y OpenGL**, que integra modelos 3D, texturas, iluminación, movimiento de cámara, curvas Bézier e interfaz gráfica de control.

---

## 1. Información del proyecto

| Campo | Información |
|---|---|
| **Nombre del proyecto** | YoutubeOpenGL |
| **Autor** | Jaider Bojacá Parada |
| **Código estudiantil** | 6014366 |
| **Lenguaje principal** | C++ |
| **API gráfica** | OpenGL |

---

## 2. Descripción general

YoutubeOpenGL es un proyecto de programación gráfica tridimensional orientado a la representación, manipulación y visualización de escenas 3D mediante OpenGL.

El proyecto integra diferentes componentes para gestionar modelos tridimensionales, aplicar materiales y texturas, controlar la iluminación y permitir la navegación dentro del entorno mediante una cámara interactiva.

También incorpora una trayectoria basada en curvas Bézier para el desplazamiento de un modelo 3D, además de una interfaz gráfica que permite consultar y modificar determinados parámetros de la escena.

---

## 3. Arquitectura del software

El proyecto utiliza una arquitectura modular, en la que las responsabilidades se distribuyen entre diferentes componentes. Esto permite organizar el código, facilitar su mantenimiento y separar las operaciones relacionadas con el renderizado, los recursos gráficos y la interacción.

### 3.1. Módulo principal

**Archivo:** `Main.cpp`

Es el punto de entrada y coordinación general del programa. Se encarga de inicializar el contexto gráfico, configurar los recursos necesarios, gestionar el ciclo principal de ejecución y coordinar la interacción entre los diferentes componentes.

También integra la lógica de cámara, iluminación, renderizado e interfaz gráfica.

### 3.2. Gestión de geometría

**Archivos:** `VAO.h`, `VAO.cpp`, `VBO.h`, `VBO.cpp`, `EBO.h`, `EBO.cpp`

Administra la información geométrica que se envía a la GPU:

- **VAO (Vertex Array Object):** organiza la configuración de los atributos de los vértices.
- **VBO (Vertex Buffer Object):** almacena los datos de los vértices.
- **EBO (Element Buffer Object):** almacena los índices utilizados para construir las primitivas geométricas.

### 3.3. Gestión de shaders

**Archivos:** `shaderClass.h`, `shaderClass.cpp`

Administra la carga, compilación y utilización de los programas de shaders.

Los shaders permiten procesar los vértices y calcular el resultado visual de los fragmentos que componen los objetos renderizados.

### 3.4. Gestión de texturas

**Archivos:** `Texture.h`, `Texture.cpp`

Se encarga de la carga y administración de texturas utilizadas por los objetos de la escena.

Se apoya en `stb_image` para la lectura de imágenes.

### 3.5. Carga de modelos 3D

**Archivos y dependencias:** `tiny_gltf_v3.h`, `tinygltf.cpp`

Integra TinyGLTF para la lectura de modelos en formato glTF, incluyendo su geometría, materiales y recursos asociados.

### 3.6. Interfaz gráfica

**Directorio:** `imgui/`

Implementa una interfaz gráfica mediante Dear ImGui. Permite visualizar los controles disponibles y modificar parámetros de interacción de la escena.

### 3.7. Renderizado e iluminación

**Archivos:** `default.vert`, `default.frag`, `light.vert`, `light.frag`, `axes.vert`, `axes.frag`, `bezier.vert`, `bezier.frag`

Contiene los programas de shaders utilizados en diferentes procesos gráficos:

- Renderizado de objetos y materiales.
- Representación de iluminación.
- Visualización de ejes.
- Representación de curvas Bézier.

---

## 4. Tecnologías y bibliotecas

| Tecnología | Función |
|---|---|
| C++ | Lenguaje de programación principal. |
| OpenGL | API para el renderizado gráfico. |
| GLFW | Creación de ventanas y gestión de entradas. |
| GLAD | Carga de funciones de OpenGL. |
| GLM | Operaciones matemáticas y transformaciones 3D. |
| TinyGLTF | Lectura de modelos en formato glTF. |
| stb_image | Carga de imágenes y texturas. |
| Dear ImGui | Desarrollo de la interfaz gráfica. |

---

## 5. Características principales

- Renderizado de escenas tridimensionales mediante OpenGL.
- Carga y visualización de modelos 3D en formato glTF.
- Aplicación de materiales y texturas.
- Sistema de iluminación configurable.
- Cámara interactiva con movimiento mediante teclado y mouse.
- Representación y seguimiento de trayectorias mediante curvas Bézier.
- Interfaz gráfica para consultar y modificar controles y parámetros de la escena.

---

## 6. Controles del programa

| Control | Función |
|---|---|
| W | Movimiento de cámara hacia adelante. |
| A | Movimiento de cámara hacia la izquierda. |
| S | Movimiento de cámara hacia atrás. |
| D | Movimiento de cámara hacia la derecha. |
| Mouse | Orientación y rotación de la cámara. |
| Shift | Incrementar la velocidad de movimiento de la cámara. |
| Flechas laterales | Movimiento horizontal de la luz. |
| Q / E | Movimiento vertical de la luz. |
| + / - | Ajuste de la intensidad de la iluminación. |
| Esc | Liberar el cursor del mouse. |
| Clic izquierdo | Recuperar el control de la cámara. |

---

## 7. Estructura del proyecto

La organización general de los archivos se distribuye de la siguiente manera:

```text
YoutubeOpenGL/
│
├── Libraries/
│   ├── include/
│   └── lib/
│
├── Modelo/
│   ├── living_room_interior_free/
│   └── paper_airplane/
│
├── imgui/
│
├── Main.cpp
│
├── VAO.cpp / VAO.h
├── VBO.cpp / VBO.h
├── EBO.cpp / EBO.h
├── Texture.cpp / Texture.h
├── shaderClass.cpp / shaderClass.h
│
├── default.vert / default.frag
├── light.vert / light.frag
├── axes.vert / axes.frag
├── bezier.vert / bezier.frag
│
├── glad.c
├── stb.cpp
├── tinygltf.cpp
│
├── YoutubeOpenGL.sln
├── YoutubeOpenGL.vcxproj
└── .gitignore
```

---

## 8. Requisitos y ejecución

### Requisitos

- Sistema operativo Windows.
- Visual Studio con herramientas de desarrollo para C++.
- Compatibilidad con OpenGL.
- Controladores gráficos actualizados.

### Ejecución

1. Descargar o clonar el repositorio.
2. Abrir el archivo `YoutubeOpenGL.sln` mediante Visual Studio.
3. Seleccionar la configuración de compilación correspondiente.
4. Compilar la solución.
5. Ejecutar el proyecto.

**Repositorio:** https://github.com/Jaider522/YoutubeOpenGL

---

## 9. Créditos y licencias

El proyecto integra bibliotecas externas y recursos gráficos de terceros.

Las licencias y condiciones de uso correspondientes deben consultarse en los archivos de licencia distribuidos con cada dependencia y recurso, especialmente los modelos 3D y sus texturas.

