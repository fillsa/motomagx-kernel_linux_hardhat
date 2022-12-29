Исходный код ядра linux на основе montavista(hardhat) для мобильной платформы motorola MotoMagx


**Подготовка зависимостей для сборки ядра**

Для сборки ядра требуется 
- скачать SDK и toolchain под платформу motomagx, например https://github.com/fillsa/motomagx-SDK-toolchain
- А также архив platform с официального репозитория от любой версии прошивки, например https://sourceforge.net/projects/motozinezn5.motorola/files/MOTOZINE%20ZN5/R6637_G_81.11.2CR_128/platform-R6637_G_81.11.2CR_128.tgz/download


**Как выбрать модель для которой требуется собрать ядро**

Для выбора под какой продукт(модель телефона) требуется собраться ядро, необходимо в корне создать ссылку на файл настроек желаемой модели. Внимание, все файлы названы по кодовым именам продуктов.

Например для zn5 создаем ссылку на файл properties.xpixl.11.2cr.

`ln -s properties.xpixl.11.2cr properties`

