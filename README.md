Исходный код ядра linux на основе montavista(hardhat) для мобильной платформы motorola MotoMagx(LinuxJava)


**1 Подготовка зависимостей для сборки ядра**

Для сборки ядра требуется 
- скачать SDK и toolchain под платформу motomagx, например https://github.com/fillsa/motomagx-SDK-toolchain
- А также архив platform от любой версии прошивки(но в рамках одной LJ версии), 
например с официального репозитория для LJ6.3 телефонов https://sourceforge.net/projects/motozinezn5.motorola/files/MOTOZINE%20ZN5/R6637_G_81.11.2CR_128/platform-R6637_G_81.11.2CR_128.tgz/download
для LJ7.1 телефонов https://sourceforge.net/projects/rokre8.motorola/files/ROKR%20E8/R6713_G_71.14.1CR_A/platform-R6713_G_71.14.1CR_A.tgz/download


**2 Как выбрать модель для которой требуется собрать ядро**

В исходниках уже есть все файлы настроек для доступных продуктов(модель телефона).
Для выбора под какой продукт(модель телефона) требуется собраться ядро, необходимо в корне создать ссылку на файл настроек желаемой модели. 
Внимание, все файлы настроек названы по кодовым именам продуктов.

Например для zn5 создаем ссылку на файл properties.xpixl.11.2cr.

`ln -s properties.xpixl.11.2cr properties`


А для E8 создаем ссылку на файл properties.elba.27.09r и т.д для желаемого продукта.

`ln -s properties.elba.27.09r properties`



**3 После подготовки зависимостей и выбора желаемой модели можно запускать сборку**

`make V=1 <name_kernel_dir>.dir`
