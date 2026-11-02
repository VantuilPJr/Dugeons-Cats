# Dungeons & Cats

**Jogo 2D em C++ com estruturas de dados aplicadas ao gameplay.**

Projeto acadêmico da disciplina de Estrutura de Dados. Um gato explora um distrito procedural inspirado em ruas brasileiras até enfrentar Caramelo, o Rei das Ruas. O objetivo técnico é tornar grafos, pilhas, filas e listas encadeadas parte do funcionamento do jogo.

**Stack:** C++17 · raylib 5.5 · CMake · CTest  
**Estado:** protótipo jogável com um piso e um chefe; instruções de compilação e testes abaixo.

## Visão geral

Jogo 2D de ação e dungeon crawler leve, desenvolvido em C++17 com raylib e CMake para a disciplina de Estrutura de Dados.

Você controla um pequeno gato laranja explorando um único piso procedural inspirado em ruas e becos urbanos brasileiros. A run termina no confronto com **Caramelo, o Rei das Ruas**.

## Funcionalidades

- distrito-dungeon procedural amplo, com seed diferente por run, 24 zonas e ruas largas interconectadas;
- mapa de `112x72` tiles (`2688x1728` pixels de mundo) e câmera top-down;
- cenário 2D brasileiro com asfalto, calçadas, muros, grafites, portões, becos, áreas abandonadas e pequenos comércios;
- protagonista inspirado no gato laranja de referência, com 24 frames direcionais de movimento e ataque;
- ratos, pássaros, cães e Caramelo com atlas animado e frames individuais exportados;
- movimento em oito direções, colisão lógica e esquiva;
- teclado, D-Pad e analógico esquerdo com dead zone radial;
- ataque de garras, crítico, knockback, cooldown e invulnerabilidade;
- ratos, pássaros e cães vira-latas com máquinas de estado próprias;
- sachê de cura, catnip e Fúria Felina temporária;
- arranhadores que concedem `ATQ +2` durante a run;
- latas de lixo com estado fechado/aberto e recompensa única;
- bolsa visual com inventário, atributos e minimapa do grafo da dungeon;
- boss com mordida, patada, investida e latido em área;
- segunda fase do boss aos 50% de HP, com agressividade e combos maiores;
- HUD, partículas, screen shake, telegraphs e áudio procedural;
- título, pausa, game over, vitória e reinício de run;
- painel acadêmico e diagnóstico de gamepad ativado por F3/Select;
- renderização interna `640x360`, ampliada para `1280x720` com filtro nearest-neighbor.

## Estruturas de dados acadêmicas

As estruturas possuem função real no jogo e têm testes automatizados.

| Estrutura | Implementação | Uso no jogo |
|---|---|---|
| Matriz | `Dungeon::TileMatrix` | Chão, paredes, corredores, desenho e colisões |
| Grafo | `DungeonGraph` | Salas como vértices e corredores como arestas |
| Pilha | `Stack<T>` manual | DFS de visita e validação da dungeon |
| Fila | `Queue<T>` manual | BFS, menor caminho e escolha da sala do boss |
| Lista encadeada | `LinkedList<T>` manual | Inventário dinâmico do jogador |

Os modificadores de Fúria Felina são recalculados sobre atributos base; os valores temporários nunca são acumulados permanentemente.

## Requisitos

- Windows 10/11;
- CMake 3.20 ou superior;
- compilador C++17 completo: Visual Studio com workload **Desktop development with C++**, MinGW-w64 ou Clang;
- Git e internet no primeiro configure.

A raylib 5.5 é obtida automaticamente pelo `FetchContent` e não precisa ser instalada globalmente.

## Compilar

Ninja ou gerador padrão:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
```

Visual Studio:

```powershell
cmake -S . -B build -G "Visual Studio 16 2019" -A x64
cmake --build build --config Debug
```

O executável de desenvolvimento fica em:

```powershell
.\build\dungeons_and_cats.exe
```

## Testar

```powershell
ctest --test-dir build --output-on-failure
```

Testes disponíveis:

- `dungeons_core_tests`: pilha, fila, lista, inventário, modificadores, grafo, DFS, BFS e geração por múltiplas seeds;
- `dungeons_gameplay_tests`: estados de inimigos, morte, fase 2 e derrota do Caramelo.

## Gerar distribuição

```powershell
cmake --install build --prefix dist --component Runtime
.\dist\dungeons_and_cats.exe
```

A pasta `dist` contém somente o executável e os assets necessários. O runtime do MinGW é incorporado estaticamente quando esse compilador é utilizado.

## Controles

| Ação | Teclado | Controle genérico / PS2 via USB |
|---|---|---|
| Movimento | WASD ou setas | Analógico esquerdo ou D-Pad |
| Ataque | Espaço ou J | Quadrado |
| Esquiva | Shift | X |
| Interagir | E | Círculo |
| Usar sachê | Q | L1 |
| Usar catnip | R | R1 |
| Abrir/fechar bolsa | Tab ou I | Triângulo |
| Pausar | Esc | Start |
| Diagnóstico | F3 | Select |

O botão Triângulo abre a bolsa. O analógico direito não possui ação de gameplay nesta versão.

## Como jogar

1. Explore as salas conectadas da dungeon.
2. Derrote inimigos para obter sachês, catnip ou tampinhas.
3. Pressione `E` perto de latas de lixo e arranhadores.
4. Use sachês quando estiver ferido e catnip em combates difíceis.
5. Procure a sala **REI DAS RUAS**.
6. Observe os telegraphs do Caramelo; todos os ataques possuem preparação, execução e recuperação.
7. Reduza o HP do boss a zero para vencer a run.

## Arte e animações

Os atlas usados em tempo de execução ficam em `assets/sprites` e `assets/tiles`. Para facilitar ajustes quadro a quadro, os 48 frames de personagem foram exportados separadamente:

- `assets/sprites/frames/player`: 24 frames do gato (4 direções, caminhada e ataque);
- `assets/sprites/frames/enemies`: 24 frames de rato, pássaro, vira-lata e Caramelo.

O código usa recortes dos atlas para manter poucas trocas de textura, mas a lógica de colisão e combate continua independente da arte.

## Diagnóstico do adaptador PS2

O `InputManager` procura os primeiros quatro gamepads e converte teclas, eixos e botões em `GameAction`. O restante do jogo não conhece índices físicos.

Pressione F3 ou Select para exibir:

- conexão, nome e índice do gamepad;
- quantidade e valores dos eixos;
- último botão pressionado e botões mantidos;
- posição, HP, sala e estado do jogo;
- dimensões da matriz;
- vértices do grafo;
- resultado da DFS/pilha e caminho BFS/fila;
- tamanho da lista encadeada do inventário;
- colliders e hitboxes.

Adaptadores PS2 genéricos podem reportar botões em ordens diferentes. Use o painel para identificar o mapeamento real sem modificar regras de gameplay.

## Arquitetura

```text
.
|-- CMakeLists.txt
|-- README.md
|-- assets
|   `-- ui
|       `-- title-background.png
|-- src
|   |-- main.cpp
|   |-- core
|   |   |-- Constants.h
|   |   |-- Game.h
|   |   `-- Game.cpp
|   |-- dungeon
|   |   |-- Dungeon.h/.cpp
|   |   |-- DungeonGenerator.h/.cpp
|   |   |-- DungeonGraph.h/.cpp
|   |   `-- Room.h
|   |-- entities
|   |   |-- Player.h/.cpp
|   |   |-- Enemy.h/.cpp
|   |   `-- CarameloBoss.h/.cpp
|   |-- input
|   |   |-- GameAction.h
|   |   `-- InputManager.h/.cpp
|   |-- items
|   |   |-- Item.h
|   |   `-- Inventory.h
|   |-- structures
|   |   |-- Stack.h
|   |   |-- Queue.h
|   |   `-- LinkedList.h
|   `-- systems
|       |-- AttributeModifiers.h
|       |-- AudioSystem.h/.cpp
|       `-- WorldObjects.h
`-- tests
    |-- CoreTests.cpp
    `-- GameplayTests.cpp
```

## Decisões e limitações

- A primeira versão possui exatamente um piso e um boss.
- A lógica não depende de pixels dos sprites para colisão.
- Sprites e cenários usam os atlas versionados em `assets`; colisão e combate permanecem independentes da arte. Consulte [o processo de arte](docs/ART_PIPELINE.md).
- A tela de título usa `assets/ui/title-background.png`, gerada com a ferramenta integrada OpenAI ImageGen; existe fallback procedural se o arquivo estiver ausente.
- Efeitos e música são sintetizados em memória, sem arquivos de áudio externos.
- A instalação parcial do Visual Studio encontrada durante o desenvolvimento não continha `rc.exe`; o build de validação utilizou WinLibs/MinGW-w64 portátil com Ninja.
- A raylib 5.5 emite um aviso de compatibilidade futura do CMake durante o configure; o aviso pertence à dependência e não afeta o build atual.

## Ideias futuras

- remapeamento de controles em arquivo;
- segundo piso e novos bosses;
- sprites animados adicionais;
- sistema de opções e volume;
- persistência de recordes por seed.

## English summary

A playable 2D C++17 game built with raylib and CMake for a Data Structures course. Graphs, manually implemented stacks, queues and linked lists support dungeon generation, traversal, shortest paths and inventory. The current scope is one procedural floor and one boss. Build and CTest instructions are documented above.
