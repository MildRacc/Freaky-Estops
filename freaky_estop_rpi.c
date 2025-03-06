#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <wiringPi.h>
#include <pthread.h>
#include "freaky_config.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

// Global variables
volatile int running = 1;
volatile int estop_triggered = 0;
int server_fd;

// Configuration variables (similar to WebServerSetup.cpp)
char allianceColor[32] = "Red";
char arenaIP[32] = "10.0.100.5";
char deviceIP[32] = "";
char arenaPort[8] = "8080";
int useDHCP = 1;

// Function prototypes
void setup(void);
void cleanup(void);
void handle_estop(void);
void* network_task(void* arg);
void* web_server_task(void* arg);
void signal_handler(int sig);
void load_settings(void);
void save_settings(void);

// Signal handler for clean termination
void signal_handler(int sig) {
    printf("Caught signal %d, cleaning up and exiting...\n", sig);
    running = 0;
    // Close server socket to unblock accept() in web server thread
    if (server_fd > 0) {
        close(server_fd);
    }
}

// GPIO interrupt for emergency stop button
void estop_interrupt(void) {
    estop_triggered = 1;
    printf("E-STOP TRIGGERED!\n");
    handle_estop();
}

void setup() {
    // Initialize wiringPi
    if (wiringPiSetup() == -1) {
        printf("WiringPi setup failed. Exiting.\n");
        exit(1);
    }
    
    // Set up E-Stop button pin as input with pull-up resistor
    pinMode(ESTOP_PIN, INPUT);
    pullUpDnControl(ESTOP_PIN, PUD_UP);
    
    // Set up interrupt
    wiringPiISR(ESTOP_PIN, INT_EDGE_FALLING, &estop_interrupt);
    
    // Set up status LED pin as output
    pinMode(STATUS_LED_PIN, OUTPUT);
    
    // Configure signal handlers for clean shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Load saved settings
    load_settings();
    
    printf("FreakyEStops initialized on Raspberry Pi\n");
}

void handle_estop() {
    // Activate emergency protocols
    digitalWrite(STATUS_LED_PIN, HIGH);  // Turn on status LED
    
    // Add your emergency stop handling code here
    // For example, send network alerts, shut down systems, etc.
    
    // In a real application, you might want to hold the system in e-stop
    // state until manually reset
}

void cleanup() {
    digitalWrite(STATUS_LED_PIN, LOW);  // Turn off status LED
    printf("Cleanup complete\n");
}

void* network_task(void* arg) {
    while(running) {
        // Here you would implement network communication functionality
        // such as sending heartbeat signals, checking for remote e-stop commands, etc.
        
        // For this example, we just blink the status LED to indicate normal operation
        if (!estop_triggered) {
            digitalWrite(STATUS_LED_PIN, HIGH);
            delay(500);
            digitalWrite(STATUS_LED_PIN, LOW);
            delay(500);
        }
    }
    return NULL;
}

// Load settings from a file (simple replacement for Preferences)
void load_settings() {
    FILE *file = fopen("Freaky_settings.txt", "r");
    if (file == NULL) {
        printf("No settings file found, using defaults\n");
        return;
    }
    
    char line[100];
    char key[50], value[50];
    
    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "%[^=]=%[^\n]", key, value) == 2) {
            if (strcmp(key, "allianceColor") == 0) {
                strncpy(allianceColor, value, sizeof(allianceColor)-1);
            } else if (strcmp(key, "arenaIP") == 0) {
                strncpy(arenaIP, value, sizeof(arenaIP)-1);
            } else if (strcmp(key, "deviceIP") == 0) {
                strncpy(deviceIP, value, sizeof(deviceIP)-1);
            } else if (strcmp(key, "arenaPort") == 0) {
                strncpy(arenaPort, value, sizeof(arenaPort)-1);
            } else if (strcmp(key, "useDHCP") == 0) {
                useDHCP = atoi(value);
            }
        }
    }
    fclose(file);
}

// Save settings to a file
void save_settings() {
    FILE *file = fopen("Freaky_settings.txt", "w");
    if (file == NULL) {
        printf("Failed to save settings\n");
        return;
    }
    
    fprintf(file, "allianceColor=%s\n", allianceColor);
    fprintf(file, "arenaIP=%s\n", arenaIP);
    fprintf(file, "deviceIP=%s\n", deviceIP);
    fprintf(file, "arenaPort=%s\n", arenaPort);
    fprintf(file, "useDHCP=%d\n", useDHCP);
    
    fclose(file);
}

// Parse HTTP query string
void parse_query_params(const char *query, char *param, char *value, size_t size) {
    char *token, *query_copy;
    query_copy = strdup(query);
    
    token = strtok(query_copy, "&");
    while (token != NULL) {
        char *eq = strchr(token, '=');
        if (eq) {
            *eq = '\0';
            if (strcmp(token, param) == 0) {
                strncpy(value, eq + 1, size - 1);
                value[size - 1] = '\0';
                break;
            }
        }
        token = strtok(NULL, "&");
    }
    
    free(query_copy);
}

// Web server thread function
void* web_server_task(void* arg) {
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[4096] = {0};
    
    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        return NULL;
    }
    
    // Set socket options for address reuse
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        return NULL;
    }
    
    // Setup server address structure
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(80);
    
    // Bind socket to port 80
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        return NULL;
    }
    
    // Listen for connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        return NULL;
    }
    
    printf("Web server started on port 80\n");
    
    while (running) {
        // Accept connection
        int client_socket;
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            if (!running) break; // Exit cleany if we're shutting down
            perror("Accept failed");
            continue;
        }
        
        // Read HTTP request
        int bytes_read = read(client_socket, buffer, sizeof(buffer));
        if (bytes_read < 0) {
            perror("Error reading request");
            close(client_socket);
            continue;
        }
        
        // Parse HTTP request (very basic parsing)
        char method[10] = {0};
        char path[100] = {0};
        char protocol[20] = {0};
        sscanf(buffer, "%s %s %s", method, path, protocol);
        
        // Get request body for POST requests
        char *body = NULL;
        if (strcmp(method, "POST") == 0) {
            body = strstr(buffer, "\r\n\r\n");
            if (body) {
                body += 4; // Move past the header-body separator
            }
        }
        
        // Handle different routes
        if (strcmp(path, "/") == 0) {
            // Home page
            char response[1024];
            snprintf(response, sizeof(response),
                "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
                "<html><body>"
                "<h1>Welcome to Freaky Estops</h1>"
                "<p>This is the initial page.</p>"
                "<button onclick=\"location.href='/setup'\">Go to Settings</button>"
                "</body></html>"
            );
            write(client_socket, response, strlen(response));
        } 
        else if (strcmp(path, "/setup") == 0) {
            // Setup page
            char checked = useDHCP ? 'c' : ' ';
            char response[2048];
            snprintf(response, sizeof(response),
                "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
                "<html><body>"
                "<h1>Setup Configuration</h1>"
                "<form action=\"/setConfig\" method=\"POST\">"
                "<label for=\"color\">Choose an alliance color: </label>"
                "<select name=\"color\" id=\"color\">"
                "<option value=\"Red\"%s>Red</option>"
                "<option value=\"Blue\"%s>Blue</option>"
                "<option value=\"Field\"%s>Field</option>"
                "</select><br><br>"
                "<input type=\"checkbox\" id=\"dhcp\" name=\"dhcp\" %s onchange=\"toggleIPInput()\">"
                "<label for=\"dhcp\">Use DHCP</label><br><br>"
                "<label for=\"ip\">Device IP: </label>"
                "<input type=\"text\" id=\"ip\" name=\"ip\" value=\"%s\"%s><br><br>"
                "<label for=\"arenaIP\">Arena IP: </label>"
                "<input type=\"text\" id=\"arenaIP\" name=\"arenaIP\" value=\"%s\">"
                "<label for=\"arenaPort\"> Port: </label>"
                "<input type=\"text\" id=\"arenaPort\" name=\"arenaPort\" value=\"%s\"><br><br>"
                "<input type=\"submit\" value=\"Submit\">"
                "</form>"
                "<script>"
                "function toggleIPInput() {"
                "  var dhcpCheckbox = document.getElementById('dhcp');"
                "  var ipInput = document.getElementById('ip');"
                "  ipInput.disabled = dhcpCheckbox.checked;"
                "}"
                "</script>"
                "</body></html>",
                strcmp(allianceColor, "Red") == 0 ? " selected" : "",
                strcmp(allianceColor, "Blue") == 0 ? " selected" : "",
                strcmp(allianceColor, "Field") == 0 ? " selected" : "",
                useDHCP ? "checked" : "",
                deviceIP,
                useDHCP ? " disabled" : "",
                arenaIP,
                arenaPort
            );
            write(client_socket, response, strlen(response));
        }
        else if (strcmp(path, "/setConfig") == 0 && strcmp(method, "POST") == 0 && body) {
            // Configuration update
            char param[32], value[32];
            
            // Parse color
            strncpy(param, "color", sizeof(param));
            parse_query_params(body, param, value, sizeof(value));
            if (strlen(value) > 0) {
                strncpy(allianceColor, value, sizeof(allianceColor)-1);
            }
            
            // Parse device IP
            strncpy(param, "ip", sizeof(param));
            parse_query_params(body, param, value, sizeof(value));
            if (strlen(value) > 0) {
                strncpy(deviceIP, value, sizeof(deviceIP)-1);
            }
            
            // Parse arena IP
            strncpy(param, "arenaIP", sizeof(param));
            parse_query_params(body, param, value, sizeof(value));
            if (strlen(value) > 0) {
                strncpy(arenaIP, value, sizeof(arenaIP)-1);
            }
            
            // Parse arena port
            strncpy(param, "arenaPort", sizeof(param));
            parse_query_params(body, param, value, sizeof(value));
            if (strlen(value) > 0) {
                strncpy(arenaPort, value, sizeof(arenaPort)-1);
            }
            
            // Parse DHCP checkbox
            useDHCP = (strstr(body, "dhcp=") != NULL);
            
            // Save settings
            save_settings();
            
            // Send response
            char response[1024];
            snprintf(response, sizeof(response),
                "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
                "<html><body>"
                "<h1>Configuration Updated</h1>"
                "<p>The configuration has been updated. Please restart the device.</p>"
                "<button onclick=\"location.href='/'\">Return Home</button>"
                "</body></html>"
            );
            write(client_socket, response, strlen(response));
        }
        else {
            // 404 Not Found
            char response[128];
            snprintf(response, sizeof(response),
                "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n"
                "<html><body><h1>404 Not Found</h1></body></html>"
            );
            write(client_socket, response, strlen(response));
        }
        
        // Close connection
        close(client_socket);
        
        // Clear buffer for next request
        memset(buffer, 0, sizeof(buffer));
    }
    
    if (server_fd > 0) {
        close(server_fd);
    }
    return NULL;
}

int main() {
    // Initialize hardware
    setup();
    
    // Create network thread
    pthread_t network_thread;
    if (pthread_create(&network_thread, NULL, network_task, NULL) != 0) {
        printf("Failed to create network thread\n");
        cleanup();
        return 1;
    }
    
    // Create web server thread
    pthread_t web_thread;
    if (pthread_create(&web_thread, NULL, web_server_task, NULL) != 0) {
        printf("Failed to create web server thread\n");
        running = 0;
        pthread_join(network_thread, NULL);
        cleanup();
        return 1;
    }
    
    // Main loop
    while (running) {
        // Main processing would go here
        delay(100);  // Small delay to reduce CPU usage
    }
    
    // Clean up before exit
    pthread_join(network_thread, NULL);
    pthread_join(web_thread, NULL);
    cleanup();
    return 0;
}
