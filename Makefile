CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lwiringPi -lpthread

TARGET = freaky_estop_rpi
SRC = freaky_estop_rpi.c
OBJ = $(SRC:.c=.o)

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/
	sudo chmod +x /usr/local/bin/$(TARGET)

# Optional: create a systemd service for auto-start
service:
	@echo "[Unit]" > freaky-estop.service
	@echo "Description=FreakyEStop Emergency Stop Service" >> freaky-estop.service
	@echo "After=network.target" >> freaky-estop.service
	@echo "" >> freaky-estop.service
	@echo "[Service]" >> freaky-estop.service
	@echo "ExecStart=/usr/local/bin/$(TARGET)" >> freaky-estop.service
	@echo "Restart=always" >> freaky-estop.service
	@echo "RestartSec=5" >> freaky-estop.service
	@echo "User=root" >> freaky-estop.service
	@echo "" >> freaky-estop.service
	@echo "[Install]" >> freaky-estop.service
	@echo "WantedBy=multi-user.target" >> freaky-estop.service
	@echo "Service file created. To install:"
	@echo "sudo cp freaky-estop.service /etc/systemd/system/"
	@echo "sudo systemctl daemon-reload"
	@echo "sudo systemctl enable freaky-estop.service"
	@echo "sudo systemctl start freaky-estop.service"
