//
// Created by yolui on 4/30/2024.
//
#include <SFML/Graphics.hpp>
#include <string>
#include <iostream>
#ifndef GRAPHICS_H
#define GRAPHICS_H

sf::Texture loadImage(std::string imageFilePath, int newWidth, int newHeight); // Carga imagen con nueva resolución
#endif //GRAPHICS_H
