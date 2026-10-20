include Makefile
build/focus-runtime-test.exe: ../tests/focus_runtime_test.c $(APP_SOURCES) build/resources.o
	$(CC) $(CFLAGS) -I. -o $@ ../tests/focus_runtime_test.c $(filter-out startup.c,$(APP_SOURCES)) build/resources.o -lcomctl32 -lgdi32 -luser32 -lddraw -lwinmm -ldsound -lcomdlg32 -lws2_32 -ldxguid
