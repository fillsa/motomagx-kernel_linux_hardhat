Исходный код ядра linux на основе montavista(hardhat) для мобильной платформы motorola MotoMagx


**Как собрать ядро с функционалом из дополнительных веток, например с поддержкой squashfs v3.4**


По разным причинам исходных код из дополнительных веток не всегда включен в основную ветку.

Для получения функционала с дополнительных веток(например поддержку squashfs v3.4 из ветки Podderzka_squashfs_3) необходимо сперва склонировать репозиторий себе.

`git clone https://github.com/fillsa/motomagx-kernel_linux_hardhat`


Перейти в папку с исходным кодом.

`cd motomagx-kernel_linux_hardhat`

Посмотреть название нужной ветки

`git branch -a`

И слить дополнительную ветку(например Podderzka_squashfs_3) в основную.

`git merge Podderzka_squashfs_3`
