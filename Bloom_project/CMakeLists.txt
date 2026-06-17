cmake_minimum_required(VERSION 3.14)
project(Bloom_project)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Добавляем пути к заголовочным файлам (ВАЖНО!)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/data_structures    
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hash
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analysis
    ${CMAKE_CURRENT_SOURCE_DIR}/sqlite
)

# Добавляем исполняемый файл с правильными путями
add_executable(${PROJECT_NAME} 
    src/main.cpp
    src/core/Bloom_project.cpp
    src/core/HyperLogLog.cpp
    src/data_structures/SkipList.cpp
    src/hash/md5.cpp
    src/hash/sha256.cpp
    src/hash/HashFunctions.cpp          # ДОБАВЛЕНО: хеш-функции
    src/core/Utils.cpp                  # ДОБАВЛЕНО: вспомогательные функции
    src/analysis/CollisionAnalyzer.cpp  # ДОБАВЛЕНО: анализ коллизий
)

# Линкуем с SFML 2.6.2 библиотеками
target_link_libraries(${PROJECT_NAME} 
    /usr/local/lib/libsfml-graphics.2.6.dylib
    /usr/local/lib/libsfml-window.2.6.dylib
    /usr/local/lib/libsfml-audio.2.6.dylib
    /usr/local/lib/libsfml-system.2.6.dylib
    sqlite3
)

# Добавляем пути к заголовочным файлам SFML
target_include_directories(${PROJECT_NAME} PRIVATE 
    /usr/local/include
)

# Копируем ресурсы в папку сборки
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    # Копируем шрифты
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/fonts
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/fonts
    
    # Копируем изображения
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/image
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/image
    
    # Копируем CSV файлы из папки resources
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/100_SalesRecords.csv
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/100_SalesRecords.csv
    
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/1000_SalesRecords.csv
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/1000_SalesRecords.csv
    
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/words.txt
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/words.txt

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/data.txt
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/data.txt
    
    # Копируем SQL файлы из resources
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/Sample-SQL-File-1000-Rows.sql
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/Sample-SQL-File-1000-Rows.sql
)

# Устанавливаем рабочую директорию для отладки (опционально)
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")