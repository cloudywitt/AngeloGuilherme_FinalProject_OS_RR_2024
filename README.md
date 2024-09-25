# Sitema de Arquivos

Sistema de arquivos simples com funcionalidades básicas de criar, ler, escrever e apagar arquivos, além da árvore de diretórios, feito para compor a nota do trabalho final da disciplina de Sistemas Operacionais I (2024.1), ofertada pelo Prof. Dr. Herbert Oliveria Rocha.

## O que é o FUSE?

O FUSE (Filesystem in USErspace) é uma interface de software utilizada para que programas de usuário possam criar e manipular um sistema de arquivos sem mexer no código do kernel. Com o FUSE, o sistema de arquivos roda como um processo na camada de usuário, com privilégios inferiores, porém mais simples e seguro.

## Implementação

O programa foi desenvolvido na seguinte configuração:
- OpenSUSE Tumbleweed versão 20240922
- Kernel 6.10.11-1-default
- GCC 14.2.0
- Make 4.4.1
- fuse 2.9.9

### Dependências

#### Ubuntu/Debian
```bash
sudo apt update
sudo apt install build-essentials pkg-config # compilador C e make mais pacotes de configuraçõese de links e bibliotecas
sudo apt install fuse # Principal do fuse
sudo apt install libfuse-dev # Bibliotecas para desenvolvimento
```

### Execução
```bash
# Entrar no diretório do código fonte
cd src

# Compilar e montar o sistema de arquivos
make

# Entrar no sistema montado
cd mnt_point
```

Após entrar no diretório no qual foi montado o SA, pode-se realizar as ações básicas:
- **touch <nome_arquivo>**: criar um arquivo vazio
- **mkdir <nome_diretorio>**: criar um diretório
- **cd <nome_diretorio>**: acessar um diretório
- **rmdir <nome_diretorio>**: remover um diretório (apenas vazio)
- **rm <nome_arquivo>**: remover um arquivo
- **nano <nome_arquivo>**: editar o texto de um arquivo
- **E mais!**

Ao final, pode-se sair e desmontar o sistema de arquivos:
```bash
# se estiver no diretório raiz, pode sair com:
cd .. 

# apagar o executável e desmontar o sistema de arquivos
make clean
```

Um detalhe é que nossa implementação faz o sistema de arquivos armazenado na memória. Ou seja, após desmontado, os arquivos e diretórios feitos são perdidos
