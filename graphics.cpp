//
// Created by yolui on 6/15/2024.
//

#include "graphics.h"

sf::Texture loadImage(std::string imageFilePath, int newWidth, int newHeight){
    sf::Texture texture;
    sf::Image originalImage;
    if (!originalImage.loadFromFile(imageFilePath)) {
        std::cerr << "No existe la imagen en: " << imageFilePath << std::endl;
        exit(-1);
    }

    unsigned int originalWidth = originalImage.getSize().x;
    unsigned int originalHeight = originalImage.getSize().y;


    sf::Image resizedImage;
    resizedImage.create(newWidth, newHeight);

    for (unsigned int x = 0; x < newWidth; ++x) {
        for (unsigned int y = 0; y < newHeight; ++y) {
            unsigned int origX = (x * originalWidth) / newWidth;
            unsigned int origY = (y * originalHeight) / newHeight;

            sf::Color pixelColor = originalImage.getPixel(origX, origY);
            resizedImage.setPixel(x, y, pixelColor);
        }
    }

    texture.loadFromImage(resizedImage);
    return texture;
}
