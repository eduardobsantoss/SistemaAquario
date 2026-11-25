# 🐠 Sistema de Automação de Aquário Inteligente

**Monitoramento e controle automatizado de aquário utilizando ESP32, sensores ambientais e interface web integrada ao Firebase.**  
Desenvolvido como parte do **Trabalho de Conclusão de Curso (TCC)** do curso de Engenharia de Computação – IFTM.

---

## 📸 Visão Geral do Projeto

O **SistemaAquario** é uma solução completa de **automação, controle e monitoramento** para aquários de pequeno porte.  
O projeto integra hardware embarcado e uma interface web moderna, permitindo acompanhar temperatura, pH e nível da água **em tempo real**, além de acionar dispositivos como **aquecedor, cascata e alimentador automático** de forma manual ou automática.

<p align="center">
  <img src="https://i.imgur.com/OMXDdfa.jpeg" alt="Dashboard do Sistema Aquário" width="800">
</p>

---

## ⚙️ Arquitetura Geral

```text
[ Sensores e Atuadores ] <---> [ ESP32 + FirebaseClient ] <---> [ Firebase Realtime Database ] <---> [ Web App (React + Vite + Tailwind) ]
    DS18B20, pH, boia                         Comunicação segura e em tempo real                        Interface moderna e responsiva
```
* **ESP32:** lê sensores e controla atuadores (aqucedor, cascata e alimentador).
* **Firebase Realtime Database:** sincroniza estados e comandos instantaneamente.
* **Interface Web**: exibe dados em tempo real e permite controle remoto.
* **Atualização OTA:** o firmware do ESP32 pode ser atualizado via rede.
---
## 🧩 Estrutura do Repositório

```text
SistemaAquario/
│
├── Esp32/                # Código do microcontrolador (PlatformIO)
│   ├── src/
│   ├── include/config/Secrets.example.h
│   ├── platformio.ini
│   └── ...
│
├── SistemaWeb/           # Interface web (React + Vite + Tailwind + Firebase)
│   ├── src/
│   ├── public/
│   ├── vite.config.ts
│   └── ...
│
└── README.md
```

---

## 💡 Principais Funcionalidades

### 🔌 ESP32

* Leitura contínua de **temperatura** (DS18B20) e **pH da água**.
* Monitoramento do **nível da água** com sensor de boia.
* Controle inteligente do **aquecedor** e **cascata** com modos:
  * **Automático:** atua conforme limites configurados.
  * **Manual:** permite comando direto via interface web.
* **Alimentador automático:** acionável manualmente ou em horários pré-definidos.
* **Logs e sincronização** com o Firebase.
* **Suporte OTA (Over-The-Air)** para atualização remota de firmware.

---

### 🌐 Sistema Web
* Login e autenticação via Firebase Authentication.
* Dashboard com:
  * Temperatura, pH e nível da água **em tempo real**.
  * Status de dispositivos (aquecedor, cascata, alimentador).
  * Controles manuais e modo automático.
  * Histórico de leituras com gráficos interativos.
* Tema claro/escuro e interface responsiva.
* Hospedagem via **GitHub Pages**.

---

## 🔧 Tecnologias Utilizadas

### 💻 Back-end embarcado
* ESP32 DevKitC
* Arduino Framework
* FirebaseClient
* DS18B20, módulo de pH, sensor de boia, relés e motor de passo

### 🌍 Interface Web
* React + TypeScript
* Vite
* TailwindCSS
* Firebase Realtime Database
* Shadcn/UI Components
* Lucide Icons
* Recharts

---

## 🚀 Como Executar Localmente

### 1️⃣ Clonar o repositório

```text
git clone https://github.com/eduardobsantoss/SistemaAquario.git
cd SistemaAquario
```

### 2️⃣ Executar o sistema web

```text
cd SistemaWeb
npm install
npm run dev
```

O app estará disponível em:
👉 http://localhost:5173

### 3️⃣ Configurar o firmware

No diretório ```/Esp32/include/config/```, copie o arquivo:

```text
Secrets.example.h → Secrets.h
```

e preencha com suas credenciais Wi-Fi e chaves Firebase.

Compile e envie para o ESP32 via **PlatformIO**.

---

## 🌎 Deploy Online

O sistema está disponível publicamente em:
👉 [Acessar Dashboard](https://eduardobsantoss.github.io/SistemaAquario "Clique para abrir")

---

## 🧠 Autor
👤 [Eduardo Barbosa dos Santos](https://www.linkedin.com/in/eduardobarbosadossantos)

🎓 Estudante de Engenharia de Computação – IFTM

💼 Atuação em automação, desenvolvimento de software e design de interfaces.

📧 [eduardo.santos@estudante.iftm.edu.br](maito:eduardo.santos@estudante.iftm.edu.br)

---

## 🏁 Licença
Este projeto está licenciado sob a Apache License 2.0. Veja o arquivo [LICENSE](LICENSE) para detalhes.
Sinta-se à vontade para estudar, modificar, expandir e contribuir.
