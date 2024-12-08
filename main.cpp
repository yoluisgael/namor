#include <bits/stdc++.h>
#include <SFML/Graphics.hpp>
#include <mutex>
#include <thread>
#include <chrono>
#include <atomic>
#include "graphics.h"
using namespace std;

// Valores de la celda
const int PARED = 0;
const int VISITADO = 1;
const int INICIO = 2;
const int FINAL = 3;
const int LIBRE = 4;
const int CORRECTO = 5;
const int CONSIDERADO = 6;

int HEURISTICA = 0;
int HEURISTICAS = 3;

const int PERIODO_MS = 1;

const long long SEED = time(nullptr);
int GRID_SIZE = 25;
int COLS = 50;
int ROWS = 50;
int BUTTON_SIZE = 50;
sf::Font font;

string bfs_clock, dfs_clock, astr_clock, dij_clock;

std::atomic<bool> detener_busqueda(false);
bool search_lock = false;

class Button {
public:
    // Constructor que recibe la posición y la textura del sprite
    Button(int pos, const sf::Texture& txtr) {
        float position = pos*(BUTTON_SIZE+10);
        buttonSprite.setPosition(5,position+5);
        // Configuración del sprite que actúa como el botón
        buttonSprite.setTexture(txtr);

        // Configurar el círculo que actuará como fondo
        float radius = BUTTON_SIZE/2 + 5; // Ajusta el tamaño del círculo según el tamaño del sprite
        backgroundCircle.setRadius(radius);
        backgroundCircle.setFillColor(sf::Color::Transparent);

        // Centrar el círculo en el sprite
        float spriteWidth = buttonSprite.getGlobalBounds().width;
        float spriteHeight = buttonSprite.getGlobalBounds().height;
        backgroundCircle.setPosition(5 + spriteWidth / 2 - radius, position+5 + spriteHeight / 2 - radius);
    }

    // Función para renderizar el botón en la ventana
    void draw(sf::RenderWindow& window) {
        window.draw(backgroundCircle); // Dibujar el círculo detrás
        window.draw(buttonSprite);     // Dibujar el sprite encima
    }

    void select(){
        backgroundCircle.setFillColor(sf::Color::Cyan);
    }

    void unselect(){
        backgroundCircle.setFillColor(sf::Color::Transparent);
    }

    // Función para verificar si el botón fue clickeado
    void handleClick(const sf::Event& event, std::function<void()> onClickAction) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);
            if (buttonSprite.getGlobalBounds().contains(mousePos)) {
                onClickAction(); // Realiza la acción si el botón es clickeado
            }
        }
    }

    void setTexture(const sf::Texture& txtr){
        buttonSprite.setTexture(txtr, true);
    }

private:
    sf::Sprite buttonSprite;
    sf::CircleShape backgroundCircle; // Círculo centrado detrás del sprite
};

class TextInputBox {
public:
    TextInputBox(sf::RenderWindow& window, sf::Font& font, int pos, int width, int height, string s)
            : window(window), font(font), isFocused(false) {
        float position = pos*(BUTTON_SIZE+10);
        inputBox.setPosition(5,position+5);
        inputBox.setSize(sf::Vector2f(width, height));
        inputBox.setOutlineThickness(2);
        inputBox.setOutlineColor(sf::Color::Black);
        inputBox.setFillColor(sf::Color::White);

        this->position.setSize(sf::Vector2f(width, height));
        this->position.setPosition(sf::Vector2f(5, position + 5));

        inputText.setFont(font);
        inputText.setCharacterSize(24);
        inputText.setFillColor(sf::Color::Black);
        inputText.setPosition(10, position+10);
        inputText.setString(s);
        userInput = s;

        sf::Event::KeyEvent keyEvent;
        textEnteredFunction = [this, keyEvent](const sf::Event& event) {
            if (event.type == sf::Event::TextEntered) {
                if (userInput.length() < 20 && isFocused && event.text.unicode >= 32 && event.text.unicode < 127) {
                    userInput += static_cast<char>(event.text.unicode);
                    inputText.setString(userInput);
                } else if (isFocused && event.text.unicode == 8 && !userInput.empty()) {
                    userInput.pop_back();
                    inputText.setString(userInput);
                }
            }
        };
    }

    void draw() {
        window.draw(inputBox);
        window.draw(inputText);
    }

    void handleEvent(const sf::Event& event) {
        if (event.type == sf::Event::MouseButtonPressed) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);
            if (position.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
                isFocused = true;
            } else {
                isFocused = false;
            }
        }

        if (event.type == sf::Event::LostFocus) {
            isFocused = false;
        }

        textEnteredFunction(event);
    }

    const std::string& getInput() const {
        return userInput;
    }

    void setPosition(int x, int y){
        inputBox.setPosition(x, y);
        inputText.setPosition(x+5, y+5);
    }

    void setText(string s){
        inputText.setString(s);
        userInput = s;
    }

private:
    sf::RenderWindow& window;
    sf::Font& font;
    sf::RectangleShape inputBox;
    sf::RectangleShape position;
    sf::Text inputText;
    bool isFocused;
    std::string userInput;
    std::function<void(const sf::Event&)> textEnteredFunction;
};

sf::Color estado(int e){
    switch(e){
        case PARED:
            return sf::Color::Black;
        case VISITADO:
            return sf::Color::Cyan;
        case INICIO:
            return sf::Color::Green;
        case FINAL:
            return sf::Color::Red;
        case LIBRE:
            return sf::Color::White;
        case CORRECTO:
            return sf::Color::Yellow;
        case CONSIDERADO:
            return {153,255,204};
        default:
            return {128, 0, 128}; // Color por defecto para estados inesperados
    }
}

void delete_canvas(vector<vector<int>>& laberinto, pair<int, int> &inicio, pair<int, int>& final){
    for(auto &x: laberinto){
        for(auto &y: x){
            y = LIBRE;
        }
    }
    inicio = {-1, -1};
    final = {-1, -1};
}

void restart(vector<vector<int>>& laberinto){
    for(auto &x: laberinto){
        for(auto &y: x){
            if(y != PARED && y != INICIO && y != FINAL)
                y = 4;
        }
    }
}

void marcar_laberinto(vector<vector<int>>& laberinto, sf::VertexArray& cuadros, sf::VertexArray& marcos, int offsetX, int offsetY, int gridsize){
    for (int i = 0; i < COLS; ++i) {
        for (int j = 0; j < ROWS; ++j) {
            // Calcular la posición de la celda
            float x = offsetX + i * gridsize;
            float y = offsetY + j * gridsize;
            // Asignar los vértices del cuadrilátero
            sf::Vertex* quad = &cuadros[(i * ROWS + j) * 4];
            quad[0].position = sf::Vector2f(x, y);
            quad[1].position = sf::Vector2f(x + gridsize, y);
            quad[2].position = sf::Vector2f(x + gridsize, y + gridsize);
            quad[3].position = sf::Vector2f(x, y + gridsize);
            // Asignar el color de la celda
            sf::Color color = estado(laberinto[i][j]);
            quad[0].color = color;
            quad[1].color = color;
            quad[2].color = color;
            quad[3].color = color;
        }
    }
    for (int i = 0; i < COLS; ++i) {
        for (int j = 0; j < ROWS; ++j) {
            // Calcular la posición de la celda
            float x = offsetX + i * gridsize;
            float y = offsetY + j * gridsize;
            sf::Vertex* lineas = &marcos[(i * ROWS + j) * 8]; // 8 vértices por celda
            // Línea superior
            lineas[0].position = sf::Vector2f(x, y);
            lineas[1].position = sf::Vector2f(x + gridsize, y);
            // Línea derecha
            lineas[2].position = sf::Vector2f(x + gridsize, y);
            lineas[3].position = sf::Vector2f(x + gridsize, y + gridsize);
            // Línea inferior
            lineas[4].position = sf::Vector2f(x + gridsize, y + gridsize);
            lineas[5].position = sf::Vector2f(x, y + gridsize);
            // Línea izquierda
            lineas[6].position = sf::Vector2f(x, y + gridsize);
            lineas[7].position = sf::Vector2f(x, y);
            // Asignar el color del marco (por ejemplo, negro)
            sf::Color colorMarco = sf::Color::Black;
            for (int k = 0; k < 8; k++) {
                lineas[k].color = colorMarco;
            }
        }
    }
}

void DFS(vector<vector<int>>& laberinto, pair<int, int> inicio, pair<int, int> final) {
    auto start = chrono::high_resolution_clock::now();
    dfs_clock = "DFS: ";

    // Si no tiene inicio, terminar
    if(inicio.first == -1 || inicio.second == -1){
        detener_busqueda = false;
        search_lock = false;
        return;
    }

    // Limpiar el laberinto
    restart(laberinto);

    // Bandera para detener búsqueda si encuentra resultado
    bool seguir_buscando = true;

    // Creamos matriz de padres
    vector<vector<pair<int, int>>> padres(COLS, vector<pair<int,int>>(ROWS, {-1, -1}));

    // Inicializamos cola con nodo inicial
    stack<pair<int, int>> pila;
    pila.push(inicio);

    // Indicadores de dirección
    int dx[] = {0, 1, 0, -1};
    int dy[] = {-1, 0, 1, 0};

    // Mientras que la pila no esté vacía
    while(!pila.empty() && seguir_buscando) {
        // Detener por intervención de usuario
        if (detener_busqueda) {
            detener_busqueda = false;
            search_lock = false;
            return;
        }

        // Extraer nodo actual del frente de la cola
        pair<int, int> nodo_actual = pila.top();
        pila.pop();

        // Marcar nodo visitado en el laberinto
        if (nodo_actual != inicio)
            laberinto[nodo_actual.first][nodo_actual.second] = VISITADO;

        // Explorar vecinos
        for(int i=0; i<4; i++){
            // Generamos vecino
            pair<int, int> vecino = {nodo_actual.first + dx[i], nodo_actual.second + dy[i]};

            // Saltar nodo si está fuera de los límites o si no es explorable
            if (vecino.first < 0 || vecino.first >= COLS || vecino.second < 0 || vecino.second >= ROWS ||
                (laberinto[vecino.first][vecino.second] != LIBRE &&
                 laberinto[vecino.first][vecino.second] != FINAL))
                continue;

            // Insertamos vecino a la pila
            pila.push(vecino);

            // Indicamos su padre como el nodo actual
            padres[vecino.first][vecino.second] = nodo_actual;

            // Marcar nodo considerado en el laberinto
            if (laberinto[vecino.first][vecino.second] != FINAL)
                laberinto[vecino.first][vecino.second] = CONSIDERADO;

                // Detener búsqueda si es el final
            else{
                seguir_buscando = false;
                break;
            }
        }

        // Dar tiempo para animación
        this_thread::sleep_for(chrono::milliseconds(PERIODO_MS));
    }

    // Reconstruir el camino desde final hasta inicio si se encontró
    if(!seguir_buscando){
        pair<int, int> current = final;
        vector<pair<int, int>> reverse;

        // Construimos el camino de final a inicio
        while(current != inicio){
            if(current != final)
                reverse.push_back(current);
            current = padres[current.first][current.second];
            if(current.first == -1 && current.second == -1){
                break;
            }
        }

        // Mostramos el camino de inicio a final
        for(auto it = reverse.rbegin(); it != reverse.rend(); it++){
            this_thread::sleep_for(chrono::milliseconds(PERIODO_MS));
            laberinto[(*it).first][(*it).second] = CORRECTO;
        }
    } else {
        cout << "No se encontró un camino al nodo final." << endl;
    }

    auto stop = chrono::high_resolution_clock::now();
    auto duration =chrono::duration_cast<chrono::milliseconds>(stop - start);
    dfs_clock += to_string(duration.count()) + "ms";
    search_lock = false;
}

void BFS(vector<vector<int>>& laberinto, pair<int, int> inicio, pair<int, int> final){
    auto start = chrono::high_resolution_clock::now();
    bfs_clock = "BFS: ";

    // Si no tiene inicio, terminar
    if(inicio.first == -1 || inicio.second == -1){
        detener_busqueda = false;
        search_lock = false;
        return;
    }

    // Limpiar el laberinto
    restart(laberinto);

    // Bandera para detener búsqueda si encuentra resultado
    bool seguir_buscando = true;

    // Creamos matriz de padres
    vector<vector<pair<int, int>>> padres(COLS, vector<pair<int,int>>(ROWS, {-1, -1}));

    // Inicializamos cola con nodo inicial
    queue<pair<int, int>> cola;
    cola.push(inicio);

    // Indicadores de dirección
    int dx[] = {0, 1, 0, -1};
    int dy[] = {-1, 0, 1, 0};

    // Mientras que la cola no esté vacía
    while(!cola.empty() && seguir_buscando) {
        // Detener por intervención de usuario
        if (detener_busqueda) {
            detener_busqueda = false;
            search_lock = false;
            return;
        }

        // Extraer nodo actual del frente de la cola
        pair<int, int> nodo_actual = cola.front();
        cola.pop();

        // Marcar nodo visitado en el laberinto
        if (nodo_actual != inicio)
            laberinto[nodo_actual.first][nodo_actual.second] = VISITADO;

        // Explorar vecinos
        for(int i=0; i<4; i++){
            // Generamos vecino
            pair<int, int> vecino = {nodo_actual.first + dx[i], nodo_actual.second + dy[i]};

            // Saltar nodo si está fuera de los límites o si no es explorable
            if (vecino.first < 0 || vecino.first >= COLS || vecino.second < 0 || vecino.second >= ROWS ||
                (laberinto[vecino.first][vecino.second] != LIBRE &&
                 laberinto[vecino.first][vecino.second] != FINAL))
                continue;

            // Insertamos vecino a la cola
            cola.push(vecino);

            // Indicamos su padre como el nodo actual
            padres[vecino.first][vecino.second] = nodo_actual;

            // Marcar nodo considerado en el laberinto
            if (laberinto[vecino.first][vecino.second] != FINAL)
                laberinto[vecino.first][vecino.second] = CONSIDERADO;

                // Detener búsqueda si es el final
            else{
                seguir_buscando = false;
                break;
            }
        }

        // Dar tiempo para animación
        this_thread::sleep_for(chrono::milliseconds(PERIODO_MS));
    }

    // Reconstruir el camino desde final hasta inicio si se encontró
    if(!seguir_buscando){
        pair<int, int> current = final;
        vector<pair<int, int>> reverse;

        // Construimos el camino de final a inicio
        while(current != inicio){
            if(current != final)
                reverse.push_back(current);
            current = padres[current.first][current.second];
            if(current.first == -1 && current.second == -1){
                break;
            }
        }

        // Mostramos el camino de inicio a final
        for(auto it = reverse.rbegin(); it != reverse.rend(); it++){
            this_thread::sleep_for(chrono::milliseconds(PERIODO_MS));
            laberinto[(*it).first][(*it).second] = CORRECTO;
        }
    } else {
        cout << "No se encontró un camino al nodo final." << endl;
    }

    auto stop = chrono::high_resolution_clock::now();
    auto duration =chrono::duration_cast<chrono::milliseconds>(stop - start);
    bfs_clock += to_string(duration.count()) + "ms";
    search_lock = false;
}


struct Nodo {
    int x, y;
    double g, h, f;
    Nodo* parent;

    Nodo(int _x, int _y) : x(_x), y(_y), g(0), h(0), f(0), parent(nullptr) {}
};

double heuristica(Nodo* a, Nodo* b){
    switch(HEURISTICA){
        case 0:
            return sqrt(pow(abs(a->x - b->x), 2) + pow(abs(a->y - b->y), 2));
        case 1:
            return pow(pow(abs(a->x - b->x), 2) + pow(abs(a->y - b->y), 2), 0.6);
        case 2:
            return abs(a->x - b->x) + abs(a->y - b->y);
        default:
            return 0;
    }
}

void AStar(vector<vector<int>>& laberinto, pair<int, int> inicio, pair<int, int> final) {
    auto start = chrono::high_resolution_clock::now();
    astr_clock = "A*: ";

    // Si no tiene inicio o fin, terminar
    if (inicio.first == -1 || inicio.second == -1 || final.first == -1 || final.second == -1) {
        detener_busqueda = false;
        search_lock = false;
        return;
    }

    // Limpiar búsqueda del laberinto
    restart(laberinto);

    // Crear nodo de inicio y nodo final
    Nodo* nodo_inicio = new Nodo(inicio.first, inicio.second);
    Nodo* nodo_final = new Nodo(final.first, final.second);

    // Inicializar valores del nodo inicio
    nodo_inicio->h = heuristica(nodo_inicio, nodo_final);
    nodo_inicio->f = nodo_inicio->g + nodo_inicio->h;

    // Crear listas
    list<Nodo*> abierto;
    list<Nodo*> cerrado;

    // Inicializar lista abierta
    abierto.push_back(nodo_inicio);

    // Bandera para detener búsqueda si encuentra resultado
    bool seguir_buscando = true;

    // Mientras que hayan nodos en la lista abierta
    while (!abierto.empty() && seguir_buscando) {

        // Detener búsqueda por intervención de usuario
        if(detener_busqueda){
            detener_busqueda = false;
            search_lock = false;
            return;
        }

        // Extraer el nodo con menor f, con funciones muy complicadas pero eficientes de entender de c++
        auto it_menor_f = min_element(abierto.begin(), abierto.end(),    // Elemento mínimo de lista abierta
                // Compara nodo a con nodo b, para ver el menor f
                                      [](const Nodo* a, const Nodo* b) {return a->f < b->f;} );
        Nodo* nodo_actual = *it_menor_f;
        abierto.erase(it_menor_f);

        // Marcar nodo actual como visitado
        if (laberinto[nodo_actual->x][nodo_actual->y] != INICIO) {
            laberinto[nodo_actual->x][nodo_actual->y] = VISITADO;
        }

        // Generar los sucesores
        for (int dx = -1; dx <= 1 && seguir_buscando; dx++) {
            for (int dy = -1; dy <= 1 && seguir_buscando; dy++) {
                // Si es el mismo nodo continuar
                if (dx == 0 && dy == 0) continue;

                // Asignamos coordenadas del hijo
                int nuevoX = nodo_actual->x + dx;
                int nuevoY = nodo_actual->y + dy;

                // Checamos si está dentro del laberinto y si no es pared
                if (nuevoX < 0 || nuevoX >= COLS || nuevoY < 0 || nuevoY >= ROWS) continue;
                if (laberinto[nuevoX][nuevoY] == PARED) continue;

                // Creamos nodo hijo
                Nodo *nodo_hijo = new Nodo(nuevoX, nuevoY);
                nodo_hijo->parent = nodo_actual;

                // Detener búsqueda y generar camino si es el nodo final
                if (nodo_hijo->x == final.first && nodo_hijo->y == final.second) {
                    Nodo *nodoActual = nodo_hijo;

                    // Generamos un vector en reversa del camino resultante
                    vector<pair<int, int>> reverse;

                    // Construimos el camino de final a inicio mientras que el nodo tenga un padre
                    while (nodoActual != nullptr) {
                        int x = nodoActual->x;
                        int y = nodoActual->y;
                        if (laberinto[x][y] != INICIO && laberinto[x][y] != FINAL) {
                            reverse.emplace_back(x, y);
                        }
                        nodoActual = nodoActual->parent;
                    }

                    // Imprimimos el camino de inicio a fin
                    for (auto it = reverse.rbegin(); it != reverse.rend(); it++) {
                        this_thread::sleep_for(chrono::milliseconds(PERIODO_MS));
                        laberinto[(*it).first][(*it).second] = CORRECTO;
                    }

                    // Detenemos la búsqueda
                    seguir_buscando = false;
                }

                // Calculamos valores del nodo hijo
                nodo_hijo->g = nodo_actual->g + sqrt(dx*dx + dy*dy);
                nodo_hijo->h = heuristica(nodo_hijo, nodo_final);
                nodo_hijo->f = nodo_hijo->g + nodo_hijo->h;

                // Checar si el nodo hijo está en la lista abierta
                auto it_open = find_if(abierto.begin(), abierto.end(),
                                       [nodo_hijo](const Nodo *p) {return p->x == nodo_hijo->x && p->y == nodo_hijo->y;});
                if (it_open != abierto.end()) {

                    // Si su f es menor lo cambiamos
                    if (nodo_hijo->f < (*it_open)->f){
                        (*it_open)->g = nodo_hijo->g;
                        (*it_open)->h = nodo_hijo->h;
                        (*it_open)->f = nodo_hijo->f;
                        (*it_open)->parent = nodo_hijo->parent;
                    }
                    delete nodo_hijo;
                    continue;
                }

                // Checar si el nodo hijo está en la lista cerrada
                auto it_closed = find_if(cerrado.begin(), cerrado.end(),
                                         [nodo_hijo](const Nodo *p) {return p->x == nodo_hijo->x && p->y == nodo_hijo->y;});
                if (it_closed != cerrado.end()) {

                    // Si su f es menor lo cambiamos
                    if(nodo_hijo->f < (*it_closed)->f){
                        (*it_closed)->g = nodo_hijo->g;
                        (*it_closed)->h = nodo_hijo->h;
                        (*it_closed)->f = nodo_hijo->f;
                        (*it_closed)->parent = nodo_hijo->parent;
                    }
                    delete nodo_hijo;
                    continue;
                }

                // Añadir nodo hijo a lista abierta
                abierto.push_back(nodo_hijo);

                // Marcar
                if (laberinto[nuevoX][nuevoY] != INICIO &&
                    laberinto[nuevoX][nuevoY] != FINAL)
                    laberinto[nuevoX][nuevoY] = CONSIDERADO;
            }
        }

        this_thread::sleep_for(chrono::milliseconds(PERIODO_MS));
        cerrado.push_back(nodo_actual);
    }

    // Liberar memoria
    for (auto it : abierto) delete it;
    for (auto it : cerrado) delete it;
    delete nodo_final;

    auto stop = chrono::high_resolution_clock::now();
    auto duration =chrono::duration_cast<chrono::milliseconds>(stop - start);
    astr_clock += to_string(duration.count()) + "ms";
    search_lock = false;
}

void Dijkstra(vector<vector<int>>& laberinto, pair<int, int> inicio, pair<int, int> final) {
    auto start = chrono::high_resolution_clock::now();
    dij_clock = "Dijkstra: ";
    if (inicio.first == -1 || inicio.second == -1) {
        detener_busqueda = false;
        search_lock = false;
        return;
    }
    restart(laberinto);

    bool seguir_buscando = true;

    // Inicializar matriz de distancias
    vector<vector<double>> distancia(COLS, vector<double>(ROWS, INT_MAX));
    distancia[inicio.first][inicio.second] = 0;

    // Inicializar matriz de visitados
    vector<vector<bool>> visitado(COLS, vector<bool>(ROWS, false));

    // Vector de padres para reconstruir el camino
    vector<vector<pair<int, int>>> padres(COLS, vector<pair<int, int>>(ROWS, {-1, -1}));

    // Cola de prioridad para almacenar nodos a visitar ordenados de menor a mayor
    priority_queue<pair<double, pair<int, int>>, vector<pair<double, pair<int, int>>>, greater<>> cola;
    cola.emplace(0, inicio);

    // Mientras que hayan nodos en la cola buscar
    while (!cola.empty() && seguir_buscando) {
        // Detener búsqueda por intervención de usuario
        if (detener_busqueda) {
            detener_busqueda = false;
            search_lock = false;
            return;
        }

        // Tiempo para mostrar animación
        this_thread::sleep_for(chrono::milliseconds(PERIODO_MS));

        // Extraer nodo de menor distancia
        pair<int, int> nodo_actual = cola.top().second;
        cola.pop();

        // Si ya fue visitado continuar
        if (visitado[nodo_actual.first][nodo_actual.second]) continue;

        // Si no ha sido visitado marcar como visitado
        visitado[nodo_actual.first][nodo_actual.second] = true;
        if (nodo_actual != inicio and nodo_actual != final)
            laberinto[nodo_actual.first][nodo_actual.second] = VISITADO;

        // Explorar vecinos
        for (int dx = -1; dx <= 1 && seguir_buscando; dx++) {
            for(int dy = -1; dy <= 1 && seguir_buscando; dy++) {

                // Saltamos si es el nodo actual
                if (dx == 0 && dy == 0) continue;

                // Asignamos coordenadas del hijo
                int nuevoX = nodo_actual.first + dx;
                int nuevoY = nodo_actual.second + dy;

                // Saltamos si está fuera del laberinto o es pared
                if (nuevoX < 0 || nuevoX >= COLS || nuevoY < 0 || nuevoY >= ROWS) continue;
                if (laberinto[nuevoX][nuevoY] == PARED) continue;

                // Calculamos la distancia del hijo
                double nuevaDistancia = distancia[nodo_actual.first][nodo_actual.second] + sqrt(dx*dx + dy*dy);

                // Si la nueva distancia es menor, reemplazamos
                if (nuevaDistancia < distancia[nuevoX][nuevoY]) {
                    distancia[nuevoX][nuevoY] = nuevaDistancia;
                    padres[nuevoX][nuevoY] = nodo_actual;
                    cola.push({nuevaDistancia, {nuevoX, nuevoY}});

                    if (laberinto[nuevoX][nuevoY] != FINAL)
                        laberinto[nuevoX][nuevoY] = CONSIDERADO;
                }

                // Detener búsqueda y marcar camino si es el final
                if(laberinto[nuevoX][nuevoY] == FINAL){
                    pair<int, int> current = final;
                    vector<pair<int, int>> reverse;

                    // Construimos el camino de final a inicio
                    while (current != inicio) {
                        if (current != final)
                            reverse.push_back(current);
                        current = padres[current.first][current.second];
                        if (current.first == -1 && current.second == -1) {
                            break;
                        }
                    }

                    // Imprimimos el camino de inicio a final
                    for (auto it = reverse.rbegin(); it != reverse.rend(); it++) {
                        this_thread::sleep_for(chrono::milliseconds(PERIODO_MS));
                        laberinto[(*it).first][(*it).second] = CORRECTO;
                    }

                    seguir_buscando = false;
                }
            }
        }
    }

    auto stop = chrono::high_resolution_clock::now();
    auto duration =chrono::duration_cast<chrono::milliseconds>(stop - start);
    dij_clock += to_string(duration.count()) + "ms";
    search_lock = false;
}

int main() {
    int offsetX;
    int offsetY;

    bool cuatro_labs = false;

    int cambio = 0;                                 // Función de click actual
    pair<int, int> inicio = {-1, -1};
    pair<int, int> final = {-1, -1};

    vector<vector<int>> laberinto(COLS, vector<int>(ROWS, 4));
    vector<vector<int>> lab_bfs;
    vector<vector<int>> lab_dfs;
    vector<vector<int>> lab_astr;
    vector<vector<int>> lab_dij;

    srand(SEED);

    sf::RenderWindow window(sf::VideoMode::getFullscreenModes()[0], "namor", sf::Style::None);
    sf::RectangleShape cell;
    cell.setOutlineColor(sf::Color::Magenta);
    cell.setOutlineThickness(1);
    if(!font.loadFromFile("C:\\fonts\\roboto.ttf")){
        cerr << "No font found" << endl;
        return -1;
    }

    sf::RectangleShape button_bar;
    button_bar.setSize(sf::Vector2f(BUTTON_SIZE + 10, window.getSize().y));
    button_bar.setFillColor(sf::Color(200,200,200));

    set<Button*> buttons;
    set<sf::Text*> texts;

    sf::Text dfs_txt, bfs_txt, astr_txt, dij_txt;
    texts.insert(&dfs_txt);
    texts.insert(&bfs_txt);
    texts.insert(&astr_txt);
    texts.insert(&dij_txt);

    for(auto t: texts)
        t->setFont(font);

    const sf::Texture draw_txtr = loadImage("images/draw.png", BUTTON_SIZE, BUTTON_SIZE);
    Button draw_btn(0, draw_txtr);

    const sf::Texture erase_txtr = loadImage("images/erase.png", BUTTON_SIZE, BUTTON_SIZE);
    Button erase_btn(1, erase_txtr);

    const sf::Texture start_txtr = loadImage("images/start.png", BUTTON_SIZE, BUTTON_SIZE);
    Button start_btn(2, start_txtr);

    const sf::Texture end_txtr = loadImage("images/end.png", BUTTON_SIZE, BUTTON_SIZE);
    Button end_btn(3, end_txtr);

    const sf::Texture bfs_txtr = loadImage("images/bfs.png", BUTTON_SIZE, BUTTON_SIZE);
    Button bfs_btn(4, bfs_txtr);

    const sf::Texture dfs_txtr = loadImage("images/dfs.png", BUTTON_SIZE, BUTTON_SIZE);
    Button dfs_btn(5, dfs_txtr);

    const sf::Texture manhattan_txtr = loadImage("images/astarM.png", BUTTON_SIZE, BUTTON_SIZE);
    const sf::Texture euclidian_txtr = loadImage("images/astarE.png", BUTTON_SIZE, BUTTON_SIZE);
    const sf::Texture euclidiant_txtr = loadImage("images/astarB.png", BUTTON_SIZE, BUTTON_SIZE);
    Button a_btn(6, manhattan_txtr);

    const sf::Texture dij_txtr = loadImage("images/dij.png", BUTTON_SIZE, BUTTON_SIZE);
    Button dij_btn(7, dij_txtr);

    const sf::Texture delete_txtr = loadImage("images/delete.png", BUTTON_SIZE, BUTTON_SIZE);
    Button delete_btn(8, delete_txtr);

    const sf::Texture resize_txtr = loadImage("images/resize.png", BUTTON_SIZE, BUTTON_SIZE);
    Button resize_btn(9, resize_txtr);

    buttons.insert(&draw_btn);
    buttons.insert(&erase_btn);
    buttons.insert(&start_btn);
    buttons.insert(&end_btn);
    buttons.insert(&bfs_btn);
    buttons.insert(&dfs_btn);
    buttons.insert(&a_btn);
    buttons.insert(&dij_btn);
    buttons.insert(&delete_btn);
    buttons.insert(&resize_btn);

    draw_btn.select();

    set<TextInputBox*> input_texts;

    TextInputBox input_cols(window, font, 10, BUTTON_SIZE, BUTTON_SIZE + 10, to_string(COLS));
    TextInputBox input_rows(window, font, 11, BUTTON_SIZE, BUTTON_SIZE + 10, to_string(ROWS));

    input_texts.insert(&input_cols);
    input_texts.insert(&input_rows);

    while (window.isOpen()) {
        // Resize según los datos actuales
        GRID_SIZE = min((window.getSize().x - BUTTON_SIZE - 10)/ COLS, window.getSize().y / ROWS);
        offsetX = BUTTON_SIZE + 10 + (window.getSize().x - BUTTON_SIZE - 10 - (GRID_SIZE * COLS)) / 2;
        offsetY = (window.getSize().y - (GRID_SIZE * ROWS)) / 2;
        cell.setSize(sf::Vector2f(GRID_SIZE, GRID_SIZE));
        if(GRID_SIZE < 4)
            cell.setOutlineThickness(0);
        else
            cell.setOutlineThickness(2);

        // Eventos
        sf::Event event;
        while (window.pollEvent(event)) {
            for(auto t: input_texts)
                t->handleEvent(event);

            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (event.type == sf::Event::KeyPressed)
                if (event.key.code == sf::Keyboard::Escape)
                    exit(0);
            if(search_lock){
                if (event.type == sf::Event::KeyPressed) {
                    if(event.key.code == sf::Keyboard::Space and !cuatro_labs){
                        detener_busqueda = true;
                    }
                }
            }

            if(!search_lock){
                if (event.type == sf::Event::KeyPressed) {
                    cuatro_labs = false;
                    if(event.key.code == sf::Keyboard::R){
                        restart(laberinto);
                    }
                    if(event.key.code == sf::Keyboard::Up){
                        HEURISTICA += 1;
                        if(HEURISTICA >= HEURISTICAS)
                            HEURISTICA = 0;
                    }
                    if(event.key.code == sf::Keyboard::Down){
                        HEURISTICA -= 1;
                        if(HEURISTICA < 0)
                            HEURISTICA = HEURISTICAS - 1;
                    }
                    if(event.key.code == sf::Keyboard::Space){
                        restart(laberinto);
                        lab_bfs = laberinto;
                        lab_dfs = laberinto;
                        lab_astr = laberinto;
                        lab_dij = laberinto;

                        cuatro_labs = true;
                        search_lock = true;

                        thread bfs_thread(BFS, ref(lab_bfs), inicio, final);
                        bfs_thread.detach();

                        thread dfs_thread(DFS, ref(lab_dfs), inicio, final);
                        dfs_thread.detach();

                        thread astar_thread(AStar, ref(lab_astr), inicio, final);
                        astar_thread.detach();

                        thread dij_thread(Dijkstra, ref(lab_dij), inicio, final);
                        dij_thread.detach();
                    }
                }

                if(sf::Mouse::isButtonPressed(sf::Mouse::Left)){
                    if(!search_lock)
                        cuatro_labs = false;
                    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                    int col = (mousePos.x - offsetX) / GRID_SIZE;
                    int row = (mousePos.y - offsetY) / GRID_SIZE;
                    if(col >= COLS or row >= ROWS) break;

                    if(mousePos.x > offsetX and mousePos.x < offsetX + (GRID_SIZE * COLS)
                       and mousePos.y > offsetY and mousePos.y < offsetY + (GRID_SIZE * ROWS)){
                        if(cambio == INICIO){
                            if(inicio.first != -1 and inicio.second != -1){
                                laberinto[inicio.first][inicio.second] = LIBRE;
                            }
                            inicio = {col, row};
                        }
                        if(cambio == FINAL){
                            if(final.first != -1 and final.second != -1){
                                laberinto[final.first][final.second] = LIBRE;
                            }
                            final = {col, row};
                        }
                        if(cambio == LIBRE){
                            if(laberinto[col][row] == INICIO){
                                inicio = {-1, -1};
                            }
                            if(laberinto[col][row] == FINAL){
                                final = {-1, -1};
                            }
                        }
                        laberinto[col][row] = cambio;
                    }
                }

                if (event.type == sf::Event::MouseButtonPressed) {
                    if(!search_lock)
                        cuatro_labs = false;
                    draw_btn.handleClick(event, [&draw_btn, &cambio, &buttons]() {
                        cambio = PARED;
                        for(auto b: buttons)
                            b->unselect();
                        draw_btn.select();
                    });
                    erase_btn.handleClick(event, [&erase_btn, &cambio, &buttons]() {
                        cambio = LIBRE;
                        for(auto b: buttons)
                            b->unselect();
                        erase_btn.select();
                    });
                    start_btn.handleClick(event, [&start_btn, &cambio, &buttons]() {
                        cambio = INICIO;
                        for(auto b: buttons)
                            b->unselect();
                        start_btn.select();
                    });
                    end_btn.handleClick(event, [&end_btn, &cambio, &buttons]() {
                        cambio = FINAL;
                        for(auto b: buttons)
                            b->unselect();
                        end_btn.select();
                    });
                    bfs_btn.handleClick(event, [&laberinto, &inicio, &final]() {
                        search_lock = true;
                        thread bfs_thread(BFS, ref(laberinto), inicio, final);
                        bfs_thread.detach();
                    });
                    dfs_btn.handleClick(event, [&laberinto, &inicio, &final]() {
                        search_lock = true;
                        thread dfs_thread(DFS, ref(laberinto), inicio, final);
                        dfs_thread.detach();
                    });
                    a_btn.handleClick(event, [&laberinto, &inicio, &final]() {
                        search_lock = true;
                        thread a_thread(AStar, ref(laberinto), inicio, final);
                        a_thread.detach();
                    });
                    dij_btn.handleClick(event, [&laberinto, &inicio, &final]() {
                        search_lock = true;
                        thread dij_thread(Dijkstra, ref(laberinto), inicio, final);
                        dij_thread.detach();
                    });
                    delete_btn.handleClick(event, [&laberinto, &inicio, &final]() {
                        delete_canvas(laberinto, inicio, final);
                    });
                    resize_btn.handleClick(event, [&laberinto, &input_rows, &input_cols, &inicio, &final]() {
                        try {
                            int newCOLS = stoi(input_cols.getInput());
                            int newROWS = stoi(input_rows.getInput());
                            vector<vector<int>> newLaberinto(newCOLS, vector<int>(newROWS, 4));
                            for (int i = 0; i < min(COLS, newCOLS); ++i) {
                                for (int j = 0; j < min(ROWS, newROWS); ++j) {
                                    newLaberinto[i][j] = laberinto[i][j];
                                }
                            }
                            laberinto = newLaberinto;
                            COLS = newCOLS;
                            ROWS = newROWS;
                            if(inicio.first >= newCOLS or inicio.second >= newROWS) inicio = {-1, -1};
                            if(final.first >= newCOLS or final.second >= newROWS) final = {-1, -1};
                        }
                        catch(...){
                            cout << "Escribe números pues\n";
                        }
                    });
                }
            }
        }

        window.clear();

        switch (HEURISTICA) {
            case 0:
                a_btn.setTexture(euclidian_txtr);
                break;
            case 1:
                a_btn.setTexture(euclidiant_txtr);
                break;
            case 2:
                a_btn.setTexture(manhattan_txtr);
                break;
        }

        if(cuatro_labs){
            sf::VertexArray cuadros(sf::Quads, COLS * ROWS * 4);
            sf::VertexArray marcos(sf::Lines, COLS * ROWS * 8); // 8 vértices por celda (2 por línea)
            int cuatro_offsetX = (window.getSize().x-BUTTON_SIZE) / 2 + offsetX / 2;
            int cuatro_offsetY = (window.getSize().y + offsetY) / 2;
            marcar_laberinto(lab_dfs, cuadros, marcos, offsetX, offsetY, GRID_SIZE/2);
            window.draw(cuadros);
            window.draw(marcos);
            marcar_laberinto(lab_bfs, cuadros, marcos, cuatro_offsetX, offsetY, GRID_SIZE/2);
            window.draw(cuadros);
            window.draw(marcos);
            marcar_laberinto(lab_astr, cuadros, marcos, offsetX, cuatro_offsetY, GRID_SIZE/2);
            window.draw(cuadros);
            window.draw(marcos);
            marcar_laberinto(lab_dij, cuadros, marcos, cuatro_offsetX, cuatro_offsetY, GRID_SIZE/2);
            window.draw(cuadros);
            window.draw(marcos);

            dfs_txt.setPosition(BUTTON_SIZE + 150, cuatro_offsetY/2);
            bfs_txt.setPosition(window.getSize().x - 200, cuatro_offsetY/2);
            astr_txt.setPosition(BUTTON_SIZE + 150, window.getSize().y - cuatro_offsetY/2);
            dij_txt.setPosition(window.getSize().x - 200, window.getSize().y - cuatro_offsetY/2);

            dfs_txt.setString(dfs_clock);
            bfs_txt.setString(bfs_clock);
            astr_txt.setString(astr_clock);
            dij_txt.setString(dij_clock);
            for(auto t: texts)
                window.draw(*t);
        }
        else{
            sf::VertexArray cuadros(sf::Quads, COLS * ROWS * 4);
            sf::VertexArray marcos(sf::Lines, COLS * ROWS * 8); // 8 vértices por celda (2 por línea)
            marcar_laberinto(laberinto, cuadros, marcos, offsetX, offsetY, GRID_SIZE);
            window.draw(cuadros);
            window.draw(marcos);
        }

        window.draw(button_bar);

        for(auto b: buttons)
            b->draw(window);

        for(auto t: input_texts)
            t->draw();

        window.display();
    }
}
