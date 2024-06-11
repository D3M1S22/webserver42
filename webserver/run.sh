CONTAINER="webserv"

IMAGE="webserver42"

# Function to check if the container exists
container_exists() {
  docker ps -a --format '{{.Names}}' | grep -w $CONTAINER > /dev/null 2>&1
}

# Function to check if the container is running
container_running() {
  docker ps --format '{{.Names}}' | grep -w $CONTAINER > /dev/null 2>&1
}

# Function to check if the image exists
image_exists() {
    docker images | grep -w $IMAGE > /dev/null 2>&1
}

# Function to stop the container
stop_container() {
  if container_running; then
    echo "Stopping container $CONTAINER..."
    docker stop $CONTAINER
    if [ $? -ne 0 ]; then
      echo "Failed to stop container $CONTAINER"
      exit 1
    fi
  else
    echo "Container $CONTAINER is not running."
  fi
}

# Function to delete the container
delete_container() {
  if container_exists; then
    echo "Deleting container $CONTAINER..."
    docker rm $CONTAINER
    if [ $? -ne 0 ]; then
      echo "Failed to delete container $CONTAINER"
      exit 1
    fi
  else
    echo "Container $CONTAINER does not exist."
  fi
}

# Function to remove the image
remove_image() {
    if image_exists; then
        echo "Removing image $IMAGE..."
        docker rmi $IMAGE
        if [ $? -ne 0 ]; then
            echo "Failed to remove image $IMAGE"
            exit 1
        fi
    else
        echo "Image $IMAGE does not exist."
    fi
}

# Function to build the image
build_image() {
  echo "Building image $IMAGE..."
  docker build -t $IMAGE "./"
  if [ $? -ne 0 ]; then
    echo "Failed to build image $IMAGE"
    exit 1
  fi
}

# Function to restart the container
restart_container() {
  echo "Restarting container $CONTAINER..."
  docker run -d --name  $CONTAINER -p 8080:8080 -p 8081:8081 $IMAGE
  if [ $? -ne 0 ]; then
    echo "Failed to restart container $CONTAINER"
    exit 1
  fi
}


# Execute the functions
stop_container
delete_container
remove_image
build_image
restart_container

echo "Starting live log detection for container $CONTAINER..."
docker logs -f $CONTAINER
