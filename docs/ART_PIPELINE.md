# Pipeline de arte 2D

Os quatro atlas foram gerados com a ferramenta integrada de geração de imagens e depois validados no jogo com filtro `TEXTURE_FILTER_POINT`.

## Referência do protagonista

A fotografia fornecida pelo autor foi usada apenas como referência visual para pelagem laranja longa, cauda volumosa e expressão curiosa. A arte do jogo é uma ilustração original em pixel art.

## Prompts finais

### Gato laranja

- Uso: personagem de action RPG top-down em pixel art moderno.
- Layout: `6x4`, com linhas `baixo`, `esquerda`, `direita`, `cima`.
- Colunas: `idle`, três quadros de caminhada, antecipação de garras e golpe de garras.
- Aparência: gato pequeno de pelo longo laranja, cauda volumosa, olhos âmbar e coleira teal.
- Restrições: exatamente 24 quadros, anatomia e paleta consistentes, fundo transparente, sem texto, grid ou elementos extras.

### Inimigos e Caramelo

- Uso: atlas de inimigos top-down em pixel art moderno.
- Layout: `6x4`, com linhas `rato`, `pássaro`, `vira-lata`, `Caramelo`.
- Colunas: `idle`, dois quadros de movimento, antecipação, ataque e dano.
- Aparência: rato urbano cinza, pombo azul-acinzentado, vira-lata tigrado com bandana vermelha e Caramelo grande com coleira roxa e medalha de coroa.
- Restrições: exatamente 24 quadros, escala crescente por categoria, fundo transparente, sem texto ou cenário.

### Distrito urbano brasileiro

- Uso: tileset top-down de ruas, becos, calçadas e áreas abandonadas.
- Layout: `6x4` com 24 tiles de chão, muros, cercas, transições e fachadas/props urbanos.
- Paleta: terracota, teal desbotado, mostarda, cinza de concreto, asfalto e vegetação.
- Elementos: asfalto, calçada portuguesa, terra, quadra pintada, tijolos, grafite abstrato, portões, vielas, entulho, caixas, lixeira, poste e pequeno comércio.
- Restrições: sem palavras, marcas, personagens ou fantasia medieval.

### Props, itens e bolsa

- Uso: atlas transparente de objetos e interface.
- Layout: `6x4`.
- Conteúdo: lixeira fechada/aberta, arranhador, caixa, poste, banco; sachê, catnip, tampinha, peixe, coração e chave; bolsa fechada/aberta e ícones de atributos; efeitos de combate, coroa e mapa.
- Restrições: um objeto centralizado por célula, fundo transparente, sem texto ou logotipos.

## Arquivos

- `assets/sprites/ginger-cat-atlas.png`
- `assets/sprites/street-enemies-atlas.png`
- `assets/sprites/urban-props-items-atlas.png`
- `assets/tiles/brazilian-urban-atlas.png`
- `assets/sprites/frames/player/*.png`
- `assets/sprites/frames/enemies/*.png`

Os 48 PNGs individuais são recortes sem reamostragem dos atlas de personagem. O jogo carrega os atlas para reduzir trocas de textura; os frames separados existem para facilitar retoques e futuras animações.
