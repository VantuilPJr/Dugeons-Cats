# Dungeons & Cats

Protótipo acadêmico de um jogo 2D de ação em C++ e raylib. O projeto está sendo desenvolvido em milestones pequenos e jogáveis; a versão atual cobre apenas o bootstrap e a camada inicial de input.

## Estado atual

- janela 16:9 em `1280x720`;
- resolução lógica fixa de `640x360`, ampliada com filtro nearest-neighbor;
- game loop com delta time e limite de segurança para frames longos;
- gato provisório representado por um retângulo laranja;
- movimento em oito direções com normalização diagonal;
- teclado, D-Pad e analógico esquerdo;
- dead zone radial configurável (`0.20`);
- detecção automática dos primeiros quatro gamepads;
- painel de diagnóstico com nome, índice, eixos e botões;
- bindings centralizados no `InputManager`.

Combate, inimigos, loot e dungeon ainda não fazem parte desta versão.

## Requisitos

- Windows 10/11;
- CMake 3.20 ou superior;
- compilador com suporte a C++17 (Visual Studio 2019+, MinGW-w64 ou Clang);
- Git e conexão com a internet na primeira configuração.

A raylib 5.5 é obtida automaticamente pelo CMake por meio de `FetchContent`; não é necessário instalá-la globalmente.

## Compilar

No PowerShell, a partir da raiz do projeto:

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

Para escolher explicitamente o Visual Studio 2019:

```powershell
cmake -S . -B build -G "Visual Studio 16 2019" -A x64
cmake --build build --config Debug
```

Para MinGW Makefiles:

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

## Executar

Geradores de configuração única (como MinGW):

```powershell
.\build\dungeons_and_cats.exe
```

Visual Studio / geradores de múltiplas configurações:

```powershell
.\build\Debug\dungeons_and_cats.exe
```

## Controles desta versão

| Ação | Teclado | Controle genérico/PS2 via adaptador |
|---|---|---|
| Movimento | WASD ou setas | Analógico esquerdo ou D-Pad |
| Mostrar/ocultar diagnóstico | F3 | Select (mapeamento lógico da raylib) |
| Fechar | Esc ou botão da janela | — |

As ações futuras já possuem bindings centralizados no `InputManager`, mas ainda não executam mecânicas: Espaço/J/Quadrado (ataque), Shift/X (esquiva), E/Círculo (interação), Q/L1 (sachê) e R/R1 (catnip).

## Diagnóstico do adaptador PS2

Conecte o adaptador antes ou depois de abrir o jogo. O painel começa visível e exibe:

- se há gamepad conectado;
- índice e nome reportados pelo sistema;
- valores de todos os eixos expostos;
- estado dos botões lógicos `0` a `17` (`P`, `H`, `R` ou `-`);
- último botão pressionado.

Adaptadores diferentes podem expor eixos e botões em ordens distintas. Anote os índices mostrados para ajustar o mapeamento sem espalhar números físicos pelo restante do jogo.

## Observações de ambiente

- O primeiro configure baixa a raylib 5.5 do repositório oficial e, por isso, requer internet.
- Se a instalação do Visual Studio estiver sem o Windows SDK (`rc.exe`), use um MinGW-w64 completo ou repare o workload **Desktop development with C++**.
- Em caminhos que contêm `&`, prefira Ninja ou um CMake instalado fora da pasta do projeto. Alguns geradores baseados em NMake podem interpretar incorretamente esse caractere quando o próprio `cmake.exe` também está dentro desse caminho.
- A raylib 5.5 emite um aviso de compatibilidade futura de CMake durante o configure; o aviso pertence à dependência e não quebra o build atual.

## Estrutura atual

```text
.
|-- CMakeLists.txt
|-- README.md
|-- .gitignore
`-- src
    |-- main.cpp
    |-- core
    |   |-- Constants.h
    |   |-- Game.h
    |   `-- Game.cpp
    `-- input
        |-- GameAction.h
        |-- InputManager.h
        `-- InputManager.cpp
```

## Próximo milestone

Validar o mapeamento com o adaptador PS2 real e, somente depois, iniciar o combate básico em uma arena fixa: player, um rato, ataque, hitbox, HP, dano e morte.
