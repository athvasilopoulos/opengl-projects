# Open GL labs

This repository contains some of the lab exercises completed in the Graphics and Virtual Reality ECE course.

## Labs Description

### 1. Texture Mapping
In this lab the object was to apply different textures on Suzanne and make them interact with each other to create an underwater wavy effect.

### 2. Mesh Manipulation
In this lab the object was to load a heart model and manipulate it using a plane. Above the plane, which can be moved by the user, the heart is opaque while below it the heart is transparent. The user can also detach the two parts of the heart, defined by the plane and also can switch between wireframe and normal view.<br>
The additional controls for this lab are:
- I/K keys for plane up/down movement
- J/L keys for plane rotation
- U/O keys for heart detachment
- T key to switch between wireframe and normal view

### 3. Standard Shading
In this lab the object was to experiment with materials and light properties, using the phong reflection model. Suzannes made up from different materials are rendered, along with a light source, and the user can tweek the light properties and position. <br>
The additional controls for this lab are:
- I/K/J/L keys to move the light around
- U/O keys to move the light up and down
- M/N keys to tweek the light intensity
- R/G/B keys to increase the corresponding base light color 

### 4. Skinning and Animation
In this lab the object was to manually do the skinning of a human model and experiment with animation. The result is the human doing the moonwalk dance.

## Build instructions

The projects can be built using CMake. The procedure is the following:
- Download [CMake](https://cmake.org/download/)
- Clone the repository
- Launch CMake. In the first line navigate to a lab's directory and in the second line enter where you want all the compiler's stuff to live, for example a new folder inside the directory named "build".
- Click configure and choose your preferred compiler/generator (The projects were developed using Visual Studio) 
- Click on generate to create the project

## General Camera Controls

On all four labs you can use the following camera controls<br>
- WASD to move the camera around
- Q/E to move the camera up and down
- Up/Down arrow for zoom effects
