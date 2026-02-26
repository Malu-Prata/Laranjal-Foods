# 🍔 Laranjal Foods: O Impasse do Delivery 🛵

Repositório destinado à **Etapa 1** do Trabalho Prático da disciplina de Sistemas Operacionais , ministrada na Universidade Federal de Pelotas (UFPel). 

Este projeto simula um ambiente de concorrência em C/C++ utilizando a biblioteca `pthread`. O objetivo principal é diagnosticar, demonstrar e resolver um problema clássico de **Deadlock** na alocação de recursos.

## 📌 O Problema: Concorrência na Laranjal Foods

Na cidade de Pelotas, o aplicativo "Laranjal Foods" exige que, para realizar uma entrega, o motoboy utilize a moto personalizada e a comida específicas daquele restaurante. O problema começa com um *bug* no sistema que ocasionalmente despacha dois entregadores para o mesmo restaurante simultaneamente.

A frota é mista e possui "rituais" conflitantes:

* **👴 O Veterano:** Foca na logística. Primeiro ele bloqueia a Moto do restaurante, e só depois tenta pegar o Pedido.


* **👦 O Novato:** É ansioso. Ele bloqueia primeiro o Pedido no balcão, para só depois ir atrás da Moto.



**🚨 O Deadlock:** Se um Veterano e um Novato chegam ao mesmo restaurante ao mesmo tempo, o Veterano pega a moto, o Novato pega o lanche, e ambos ficam travados eternamente aguardando o recurso que o outro está segurando.

---

## 🛠️ A Nossa Solução (Prevenção e Resolução)

Para evitar que as *threads* congelem a simulação inteira, implementamos um sistema robusto de prevenção e resolução de deadlocks focado em "regras de negócio", executado durante a execução do simulador.

Quando um possível deadlock é detectado (após múltiplas tentativas falhas de usar `pthread_mutex_trylock` no segundo recurso), as seguintes regras são avaliadas para decidir quem deve **desistir do seu recurso e ceder a vez**:

1. **💰 Capitalismo Justo (Lucro):** O entregador que acumulou mais dinheiro nas entregas anteriores desiste para dar chance ao outro.
2. **⭐ Sistema de Prioridade (Aging):** Se um entregador desiste muito, sua prioridade aumenta para garantir que ele não sofra de inanição (*starvation*).
3. **Respeito aos Mais Velhos:** Em caso de empate, o Novato cede a vez para o Veterano.

**Sistema de Reservas:** Para evitar *Race Conditions* onde um terceiro entregador "rouba" o recurso que acabou de ser cedido, implementamos uma matriz de reservas (`reservaPedido` e `reservaMoto`). Quem cede o recurso já o deixa reservado nominalmente para o seu adversário!

---

## ⚙️ Como Executar

O projeto foi construído utilizando C++ com a biblioteca `pthread`. Siga os passos abaixo em um ambiente Linux (ou WSL):

**1. Clone e compile o projeto:**

```bash
# Compile o arquivo principal juntamente com a flag de pthreads
g++ -o delivery main.cpp -lpthread -Wall

```

*(Nota: Certifique-se de que os arquivos `.h` da pasta `headers/` estejam devidamente referenciados e preenchidos no seu diretório local).*

**2. Execute o programa:**

```bash
./delivery

```

---

## 🖥️ Acompanhando a Simulação (Logs)

O console imprimirá o status em tempo real de forma organizada e tabular para facilitar a visualização por restaurante.
As cores ajudarão a identificar quem está agindo:

* 🟪 **Roxo:** Entregador Veterano
* 🟨 **Amarelo:** Entregador Novato

Acompanhe as mensagens de *"pegou recurso"*, *"desistiu"* (acionamento da nossa resolução de deadlock) e *"fez a entrega"* para entender como o nosso algoritmo gerencia os impasses!

## Alunos
*  Diogo Kruger Souto;
*  Maria Luiza Batista Prata;
*  Milena Alves Ferreira.
