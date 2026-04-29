# Especificação da Implementação

> [!CAUTION]
> - Você <ins>**não pode utilizar ferramentas de IA para escrever esta
>   especificação**</ins>

## Integrantes da dupla

- **Aluno 1 - Nome**: <mark>`Caio Felipe Ferreira Nunes`</mark>
- **Aluno 1 - Cartão UFRGS**: <mark>`00588024`</mark>

- **Aluno 2 - Nome**: <mark>`João Kenji Suwa`</mark>
- **Aluno 2 - Cartão UFRGS**: <mark>`00587808`</mark>

## Detalhes do que será implementado

- **Título do trabalho**: <mark>`Doom 1 - 3D Version`</mark>
- **Parágrafo curto descrevendo o que será implementado**: 
Será implementado um jogo versão 3D baseado na [primeira fase](https://www.youtube.com/watch?v=MnqLJpgq7jc) do Doom 1

## Especificação visual

### Vídeo - Link

> [!IMPORTANT]
> - Coloque aqui um link para um vídeo que mostre a aplicação gráfica
>   de referência que você vai implementar. **Sua implementação deverá
>   ser o mais parecido possível com o que é mostrado no vídeo (mais
>   detalhes abaixo).**
> - **Você não pode escolher como referência: (1) algum trabalho realizado
>   por outros alunos desta disciplina, em semestres anteriores. (2) Minecraft.**
> - Por exemplo, você pode colocar um vídeo de um jogo que você gosta,
>   e seu trabalho final será uma re-implementação do jogo.
> - O vídeo pode ser um link para YouTube, Google Drive, ou arquivo mp4 dentro
>   do próprio repositório. Mas, garanta que qualquer um tenha
>   permissão de acesso ao vídeo através deste link.

<mark>[Primeira Fase](https://www.youtube.com/watch?v=MnqLJpgq7jc)</mark>

### Vídeo - Timestamp

> [!IMPORTANT]
> - Coloque aqui um **intervalo de ~30 segundos** do vídeo acima, que
>   será a base de comparação para avaliar se o seu trabalho final
>   conseguiu ou não reproduzir a referência.

- **Timestamp inicial**: <mark>`<0:10>`</mark>
- **Timestamp final**: <mark>`<0:40>`</mark>

### Imagens

> [!IMPORTANT]
> - Coloquei aqui **três imagens** capturadas do vídeo acima, que você
>   irá usar como ilustração para as explicações que vêm abaixo.
![Itens Dropador](/images/DoomItens.png "Itens Dropados.")
![Inimigos](/images/DoomEnime.png "Inimigos.")
![Sala Doom](/images/DoomRoom.png "Visao da sala.")


## Especificação textual

> [!IMPORTANT]
> - Coloquei aqui **três imagens** capturadas do vídeo acima, que você
>   irá usar como ilustração para as explicações que vêm abaixo.
![Itens Dropador](/images/DoomItens.png "Itens Dropados.")
![Inimigos](/images/DoomEnime.png "Inimigos.")
![Sala Doom](/images/DoomRoom.png "Visao da sala.")

Para cada um dos requisitos abaixo (detalhados no [Enunciado do Trabalho final - Moodle](https://moodle.ufrgs.br/mod/assign/view.php?id=6018620)), escreva um parágrafo **curto** explicando como este requisito será atendido, apontando itens específicos do vídeo/imagens que você incluiu acima que atendem estes requisitos.

### Malhas poligonais complexas
A aplicação utilizará modelos 3D com malhas poligonais complexas para armas, inimigos, itens, etc.

### Transformações geométricas controladas pelo usuário
O jogador poderá controlar transformações geométricas pela movimentação do personagem e da câmera, além da interação com objetos e inimigos.

### Diferentes tipos de câmeras
Será implementada uma câmera livre em 1ª pessoa (FPS) e outra free-roam (espectador), possibilitando ao jogador andar livremente pelo cenário enquanto pausado.

### Instâncias de objetos
O mesmo modelo 3D do inimigo será reutilizado diversas vezes ao longo da fase, permitindo múltiplas instâncias do mesmo objeto.

### Testes de intersecção
O personagem irá portar uma arma que pode disparar balas contra inimigos/objetos na cena; caso acerte, serão feitos testes de intersecção entre o projétil e o objeto.

### Modelos de Iluminação em todos os objetos
Todos os objetos possuirão iluminação baseada em shaders, considerando diferentes fontes de luz ao longo do mapa.

### Mapeamento de texturas em todos os objetos
Todo objeto, sejam eles armas, armaduras ou inimigos, terá suas próprias texturas, mantendo um "banco" de texturas para evitar repetição visual.

### Movimentação com curva Bézier cúbica
Cada inimigo terá sua trajetória definida por uma curva de Bézier cúbica, mantendo uma movimentação suave.

### Animações baseadas no tempo ($\Delta t$)
Todas as movimentações da aplicação serão baseadas no tempo decorrido entre frames, utilizando a técnica de delta time. Dessa forma, a velocidade dos objetos e da câmera permanece consistente independentemente da taxa de quadros por segundo (FPS) ou do desempenho do hardware.

## Limitações esperadas

> [!IMPORTANT]
> - Coloque aqui uma lista de detalhes visuais ou de interação que
>   aparecem no vídeo e/ou imagens acima, mas que você **não pretende
>   implementar** ou que você **irá implementar parcialmente**.
> - Para cada item, **explique por que** não será implementado ou por
>   que será implementado parcialmente.

- **Hotbar:** Será feita uma versão simplificada da hotbar inferior do jogador, pois esse não é o foco principal do trabalho.
- **Fase parcial:** Devido ao tamanho da fase e aos detalhes presentes ao longo dela, será implementada apenas parte do cenário, para que seja possível manter o foco nos detalhes e nos conceitos de Computação Gráfica.
