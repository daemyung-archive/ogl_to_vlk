# OpenGL사용자를 위한 Vulkan 튜토리얼

이 튜토리얼에선 그래픽스에 대한 지식을 언급하진 않으며 OpenGL사용자가 Vulkan을 사용하기 위해 필요한 지식을 다룹니다.

## 시작하기

작성된 예제들을 실행하기 필요한 프로그램을 설치하고 소스를 다운받는법을 설명합니다.

### 전제 조건

예제를 다운받고 빌드하기 위해선 IDE, GIT, CMake가 필요합니다.

### 소스 받기

이 프로젝트는 다른 프로젝트에 의존성이 있기 때문에 재귀적으로 소스를 받아야 합니다.

```
git clone https://github.com/daemyung/ogl_to_vlk.git
```

## 프로젝트 파일 생성

CMake를 사용해 선호하는 프로젝트 파일을 생성합니다.

```
mkdir build
cd build
cmake -G Xcode ..
```

## 목차

삼각형을 그리기 위해 반드시 알아야하는 내용에 대해 설명합니다.
원래 책으로 출판하려 했으나 흥미를 끌만한 내용이 부족하고 데모가 너무 간단하다는 의견이 있어서 출판을 포기하고 작성된 [문서](https://docs.google.com/document/d/1SWICNMyYS0egmK_vxbDvWTg3-vRuB9BdEQzlVJEK7rc/edit?usp=sharing)를 공개합니다.

### 01장. Vulkan 이란?

* 설명: https://blog.naver.com/dmatrix/221808847792

### 02장. Vulkan 런타임 컴파일러

* 설명: https://blog.naver.com/dmatrix/221826523428
* 소스: https://github.com/daemyung/ogl_to_vlk/blob/master/chapter02/main.cpp

### 03장. Vulkan 시작하기

* 설명: https://blog.naver.com/dmatrix/221809475599
* 소스: https://github.com/daemyung/ogl_to_vlk/blob/master/chapter03/main.cpp

### 04장. Vulkan과 OS의 만남

* 설명: https://blog.naver.com/dmatrix/221815133123
* 소스: https://github.com/daemyung/ogl_to_vlk/blob/master/chapter04/main.cpp

### 05장. Vulkan의 첫 화면 출력

* 설명: https://blog.naver.com/dmatrix/221818207422
* 소스: https://github.com/daemyung/ogl_to_vlk/blob/master/chapter05/main.cpp

### 06장. Vulkan의 동기화

* 설명: https://blog.naver.com/dmatrix/221822218570
* 소스: https://github.com/daemyung/ogl_to_vlk/blob/master/chapter06/main.cpp

### 07장. Vulkan의 렌더패스와 프레임버퍼

* 설명: https://blog.naver.com/dmatrix/221832644217
* 소스: https://github.com/daemyung/ogl_to_vlk/blob/master/chapter07/main.cpp

### 08장. Vulkan에서의 첫 삼각형

* 설명: https://blog.naver.com/dmatrix/221835320809
* 소스: https://github.com/daemyung/ogl_to_vlk/blob/master/chapter08/main.cpp

### 09장.Vulkan의 메모리

* 설명: https://blog.naver.com/dmatrix/221837077087
* 소스: https://github.com/daemyung/ogl_to_vlk/blob/master/chapter09/main.cpp

### 10장. Vulkan의 버텍스 버퍼

* 설명: https://blog.naver.com/dmatrix/221839073030
* 소스: https://github.com/daemyung/ogl_to_vlk/blob/master/chapter10/main.cpp

### 11장. Vulkan의 인덱스 버퍼

* 설명: https://blog.naver.com/dmatrix/221846278529
* 소스: https://github.com/daemyung/ogl_to_vlk/blob/master/chapter11/main.cpp

### 12장. Vulkan의 유니폼 버퍼

* 설명: https://blog.naver.com/dmatrix/221849060447
* 소스: https://github.com/daemyung/ogl_to_vlk/blob/master/chapter12/main.cpp

### 13장. Vulkan의 텍스쳐

* 설명: https://blog.naver.com/dmatrix/221856392298
* 소스: https://github.com/daemyung/ogl_to_vlk/blob/master/chapter13/main.cpp

### 14장. Vulkan의 파이프라인 베리어

* 설명: https://blog.naver.com/dmatrix/221858062590

### 15장. Vulkan의 버퍼링

* 설명: https://blog.naver.com/dmatrix/221859995257
* 소스: https://github.com/daemyung/ogl_to_vlk/blob/master/chapter15/main.cpp

## 저자

* **장대명** (dm86.jang@gmail.com)
