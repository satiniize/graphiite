valgrind --tool=memcheck --leak-check=full --track-origins=yes --show-leak-kinds=all --verbose --suppressions=vulkan.supp ./build/Debug/software-renderer
