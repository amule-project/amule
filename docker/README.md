# Amule in docker

Multi platform docker image for amule.

Based on the work of:
+ x86: https://github.com/tchabaud/dockerfiles/tree/master/amule  
+ armv7: https://github.com/synopsis8/dockerfiles/tree/master/amule  


## Multiple architecture build

```
# In an amd64
AMULE_VERSION=2.3.2 
docker build --no-cache --build-arg AMULE_VERSION=${AMULE_VERSION} -t amule:${AMULE_VERSION}-amd64 .
docker push amule:${AMULE_VERSION}-amd64

# In an arm32v7 (raspberry pi 3)
AMULE_VERSION=2.3.2 
docker build --no-cache --build-arg AMULE_VERSION=${AMULE_VERSION} -t amule:${AMULE_VERSION}-arm32v7 .
docker push amule:${AMULE_VERSION}-arm32v7

# In any machine
AMULE_VERSION=2.3.2
docker manifest create amule:${AMULE_VERSION} --amend amule:${AMULE_VERSION}-amd64 --amend amule:${AMULE_VERSION}-arm32v7
docker manifest inspect amule:${AMULE_VERSION} | less
docker manifest push amule:${AMULE_VERSION}

```


## Bibliography

https://www.docker.com/blog/multi-arch-build-and-images-the-simple-way/