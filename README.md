# clang.tidy

** clang-tidy maiken module ** 

Link phase module

## Prerequisites
  [maiken](https://github.com/Dekken/maiken)


## Usage

```yaml
mod:
  - name: clang.tidy
    version: master
    arg:
        link:
            args: -DArg       #(optional, default="")
            checks: CSV       #(optional, default="", see: [link](https://clang.llvm.org/extra/clang-tidy/checks/list.html)) 
            config: file      #(optional, default='.clang-tidy')
            types: cpp:cxx:cc #(optional, default=cpp:cxx:cc)
```  

## Building
  
  Windows cl:

    mkn clean build -tSa -EHsc -dp test
  	mkn clean build -tSa -EHsc 
  	mkn run -p test


  *nix gcc:

    mkn clean build -tSa "-O2 -fPIC" -dp test -l "-pthread -ldl"
    mkn clean build -tSa "-O2 -fPIC" -l "-pthread -ldl"
    mkn run -p test
    
