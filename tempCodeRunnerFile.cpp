#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <limits>
#include <cstdlib>
#include <ctime>
#include <map> 
#include <stdexcept> // Include for exceptions

using namespace std;

// --- Helper Functions and Constants ---

// Constants for file names
const string TRAIN_FILE = "trains_data.txt";
const string BOOKING_FILE = "bookings_data.txt";
const string PNR_FILE = "pnr_counter.txt";
const string USER_FILE = "users_data.txt"; 

// Function to clear input buffer after failed read
void clearInputBuffer() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

// Function to validate date format (Simplified MM/DD/YYYY)
bool isValidDate(const string& date) {
    if (date.length() != 10 || date[2] != '/' || date[5] != '/') return false;
    // Basic numerical checks
    if (!isdigit(date[0]) || !isdigit(date[1]) || !isdigit(date[3]) || 
        !isdigit(date[4]) || !isdigit(date[6]) || !isdigit(date[7]) || 
        !isdigit(date[8]) || !isdigit(date[9])) return false;
    // Simple month/day check
    try {
        int month = stoi(date.substr(0, 2));
        int day = stoi(date.substr(3, 2));
        if (month < 1 || month > 12 || day < 1 || day > 31) return false;
    } catch (...) {
        return false;
    }
    return true;
}

// --- 1. Passenger Class (Encapsulation) ---
class Passenger {
private:
    string name;
    int age;
    string gender; // M/F/O

public:
    // Constructor
    Passenger(const string& n, int a, const string& g)
        : name(n), age(a), gender(g) {}

    // Default Constructor (required for vector operations)
    Passenger() : name(""), age(0), gender("") {}

    // Getters
    string getName() const { return name; }
    int getAge() const { return age; }
    string getGender() const { return gender; }

    // Display
    void displayPassenger() const {
        cout << "    Name: " << name << ", Age: " << age 
             << ", Gender: " << gender << endl;
    }

    // Serialization for File Persistence
    string serialize() const {
        // Format: Name|Age|Gender
        return name + "|" + to_string(age) + "|" + gender;
    }
};

// --- 2. SeatAllocation Structure (Data Management) ---
struct SeatAllocation {
    string date;
    int availableSeats;

    string serialize() const {
        return date + "|" + to_string(availableSeats);
    }
    
    static SeatAllocation deserialize(const string& data) {
        stringstream ss(data);
        string segment;
        vector<string> parts;
        while (getline(ss, segment, '|')) {
            parts.push_back(segment);
        }
        try {
            if (parts.size() == 2) {
                // Ensure conversion is safe if deserialize is called directly
                return {parts[0], stoi(parts[1])};
            }
        } catch (const std::exception& e) {
            cerr << "[Error] SeatAllocation deserialization failed: " << e.what() << endl;
        }
        return {"", 0}; // Error case
    }
};

// --- NEW CLASS 3: Route (Schedule/Timetable Abstraction) ---
class Route {
private:
    string sourceStation;
    string destinationStation;
public:
    Route(const string& src, const string& dest)
        : sourceStation(src), destinationStation(dest) {}

    string getSource() const { return sourceStation; }
    string getDestination() const { return destinationStation; }

    // Serialization: Src|Dest
    string serialize() const {
        return sourceStation + "|" + destinationStation;
    }
    static Route deserialize(const string& data) {
        stringstream ss(data);
        string src, dest;
        getline(ss, src, '|');
        getline(ss, dest, '|');
        return Route(src, dest);
    }
};


// --- 4. Train Base Class (Abstraction/Polymorphism) ---
class Train {
protected:
    string trainNumber;
    string trainName;
    Route route; // Using the new Route class
    int totalSeats;
    double baseFare;
    
    vector<SeatAllocation> seatMap; 

public:
    // Constructor (Updated to use Route object)
    Train(const string& num, const string& name, const Route& r, int seats, double fare)
        : trainNumber(num), trainName(name), route(r), totalSeats(seats), baseFare(fare) {
        time_t t = time(0);
        tm* now = localtime(&t);
        string today = to_string(now->tm_mon + 1) + "/" + to_string(now->tm_mday) + "/" + to_string(now->tm_year + 1900);
        seatMap.push_back({today, seats});
    }

    // Pure virtual function (Polymorphism)
    virtual void displayDetails() const = 0; 
    
    // Virtual function to serialize class data
    virtual string serialize() const = 0;

    // Getters
    string getTrainNumber() const { return trainNumber; }
    string getTrainName() const { return trainName; }
    string getSource() const { return route.getSource(); }
    string getDestination() const { return route.getDestination(); }
    int getTotalSeats() const { return totalSeats; }
    double getBaseFare() const { return baseFare; }

    // Seat management using date
    int getAvailableSeats(const string& date) {
        for (auto& alloc : seatMap) {
            if (alloc.date == date) {
                return alloc.availableSeats;
            }
        }
        // If date not found, assume full availability and add the date
        if (isValidDate(date)) {
             seatMap.push_back({date, totalSeats});
             return totalSeats;
        }
        return -1; // Invalid date
    }

    bool bookSeat(const string& date, int count = 1) {
        for (auto& alloc : seatMap) {
            if (alloc.date == date) {
                if (alloc.availableSeats >= count) {
                    alloc.availableSeats -= count;
                    return true;
                }
                return false; // Not enough seats
            }
        }
        // Date not found: treat as new date
        if (isValidDate(date) && count <= totalSeats) {
            seatMap.push_back({date, totalSeats - count});
            return true;
        }
        return false;
    }
    
    void cancelSeat(const string& date, int count = 1) {
        for (auto& alloc : seatMap) {
            if (alloc.date == date) {
                alloc.availableSeats += count;
                if (alloc.availableSeats > totalSeats) {
                    alloc.availableSeats = totalSeats; 
                }
                return;
            }
        }
    }

    // Serialize seat map (complex persistence part)
    string serializeSeatMap() const {
        string data = to_string(seatMap.size()) + ":";
        for (const auto& alloc : seatMap) {
            data += alloc.serialize() + ";";
        }
        return data; // Format: Count:Date1|Seats1;Date2|Seats2;
    }

    // Deserialize seat map
    void deserializeSeatMap(const string& data) {
        seatMap.clear();
        size_t count_end = data.find(':');
        if (count_end == string::npos) return;

        string count_str = data.substr(0, count_end);
        int count = 0;
        try {
            count = stoi(count_str);
        } catch (...) {
            return; // Crash defense
        }
        
        string segment = data.substr(count_end + 1);
        stringstream ss(segment);
        string alloc_data;
        
        for (int i = 0; i < count && getline(ss, alloc_data, ';'); ++i) {
            seatMap.push_back(SeatAllocation::deserialize(alloc_data));
        }
    }
    
    // Virtual destructor
    virtual ~Train() {}
};

// --- 5. ExpressTrain Derived Class (Inheritance) ---
class ExpressTrain : public Train {
private:
    bool hasPantryCar;

public:
    // Constructor (Updated to use Route object)
    ExpressTrain(const string& num, const string& name, const Route& r, int seats, double f, bool pantry)
        : Train(num, name, r, seats, f), hasPantryCar(pantry) {}

    // Overridden function (Polymorphism)
    void displayDetails() const override {
        cout << "    Train Number: " << trainNumber << endl;
        cout << "    Train Name: " << trainName << " (EXPRESS)" << endl;
        cout << "    Route: " << route.getSource() << " -> " << route.getDestination() << endl;
        cout << "    Total Seats: " << totalSeats << ", Base Fare: ₹" << fixed << setprecision(2) << baseFare << endl;
        cout << "    Pantry Car: " << (hasPantryCar ? "Yes" : "No") << endl;
    }
    
    // Overridden serialization
    string serialize() const override {
        // Format: TYPE|Num|Name|RouteData|TotalSeats|BaseFare|Pantry|SeatMapData
        return "EXPRESS|" + trainNumber + "|" + trainName + "|" + route.serialize() + "|" + 
               to_string(totalSeats) + "|" + to_string(baseFare) + "|" + to_string(hasPantryCar) + "|" + serializeSeatMap();
    }
    
    bool getPantryStatus() const { return hasPantryCar; }
};

// --- 6. Booking/Ticket Class ---
class Booking {
private:
    string pnrNumber; 
    string trainNumber;
    string dateOfJourney;
    vector<Passenger> passengers;
    double totalFare;
    string status; // Confirmed/Cancelled

public:
    // Constructor
    Booking(const string& pnr, const string& tNum, const string& date, const vector<Passenger>& p_list, double fare)
        : pnrNumber(pnr), trainNumber(tNum), dateOfJourney(date), passengers(p_list), totalFare(fare), status("Confirmed") {}

    // Default Constructor for File Loading
    Booking() : pnrNumber(""), trainNumber(""), dateOfJourney(""), totalFare(0.0), status("") {}

    // Getters
    string getPNR() const { return pnrNumber; }
    string getTrainNumber() const { return trainNumber; }
    string getDate() const { return dateOfJourney; }
    double getTotalFare() const { return totalFare; }
    string getStatus() const { return status; }
    
    // Mutator
    void setStatus(const string& newStatus) { status = newStatus; }

    // Display
    void displayBooking() const {
        cout << "\n    --- Booking Details (PNR: " << pnrNumber << ") ---" << endl;
        cout << "    Train Number: " << trainNumber << ", Date: " << dateOfJourney << endl;
        cout << "    Booking Status: " << status << endl;
        cout << "    Total Fare Paid: ₹" << fixed << setprecision(2) << totalFare << endl;
        cout << "    Passengers (" << passengers.size() << "):" << endl;
        for (const auto& p : passengers) {
            p.displayPassenger();
        }
        cout << "    -------------------------------------------------" << endl;
    }
    
    int getNumPassengers() const {
        return passengers.size();
    }
    
    // Serialization for File Persistence
    string serialize() const {
        string p_data = "";
        for (const auto& p : passengers) {
            p_data += p.serialize() + "&"; // Passenger separator
        }
        if (!p_data.empty()) p_data.pop_back(); // Remove trailing separator

        // Format: PNR|TrainNum|Date|Fare|Status|PassengerCount|PassengerData
        return pnrNumber + "|" + trainNumber + "|" + dateOfJourney + "|" + to_string(totalFare) + "|" + status + 
               "|" + to_string(passengers.size()) + "|" + p_data;
    }

    // Deserialization (Static or standalone helper recommended for production)
    static Booking deserialize(const string& data) {
        stringstream ss(data);
        string segment;
        vector<string> parts;
        while (getline(ss, segment, '|')) {
            parts.push_back(segment);
        }

        if (parts.size() < 7) return Booking(); // Invalid format
        
        Booking b;
        try {
            b.pnrNumber = parts[0];
            b.trainNumber = parts[1];
            b.dateOfJourney = parts[2];
            b.totalFare = stod(parts[3]);
            b.status = parts[4];
            int p_count = stoi(parts[5]);
            string p_data_str = parts[6];

            stringstream pss(p_data_str);
            string p_segment;
            
            // Deserialize Passengers
            for (int i = 0; i < p_count && getline(pss, p_segment, '&'); ++i) {
                stringstream psss(p_segment);
                string p_part;
                vector<string> p_parts;
                while(getline(psss, p_part, '|')) {
                    p_parts.push_back(p_part);
                }
                if (p_parts.size() == 3) {
                    b.passengers.emplace_back(p_parts[0], stoi(p_parts[1]), p_parts[2]);
                }
            }
        } catch (const std::exception& e) {
            cerr << "[Error] Booking deserialization failed: " << e.what() << ". Skipping record." << endl;
            return Booking(); // Return empty booking on exception
        }
        return b;
    }
};

// --- NEW CLASS 7: PNRGenerator ---
class PNRGenerator {
private:
    long long currentPNR = 100000000000;
    
    void savePNR() const {
        ofstream pnrFile(PNR_FILE);
        if (pnrFile.is_open()) {
            pnrFile << currentPNR;
            pnrFile.close();
        }
    }

    void loadPNR() {
        ifstream pnrFile(PNR_FILE);
        if (pnrFile.is_open()) {
            // FIX: Robust check to handle empty or malformed file content
            string pnr_str;
            if (pnrFile >> pnr_str && !pnr_str.empty()) {
                try {
                    currentPNR = stoll(pnr_str);
                } catch (const std::exception& e) {
                    // Fallback if file content is non-numeric or malformed
                    currentPNR = 100000000000;
                    cerr << "[Warning] PNR file corrupted (" << e.what() << "). Resetting counter." << endl;
                }
            }
        }
        if (currentPNR < 100000000000) {
            currentPNR = 100000000000;
        }
    }
    
public:
    PNRGenerator() {
        loadPNR();
    }

    string generate() {
        currentPNR++;
        savePNR();
        return to_string(currentPNR);
    }
};

// --- NEW CLASS 8: User Base Class (Polymorphism) ---
class User {
protected:
    string username;
    string password;
    string role;
public:
    User(const string& u, const string& p, const string& r) : username(u), password(p), role(r) {}
    
    virtual void displayMenu() const = 0;
    string getUsername() const { return username; }
    string getRole() const { return role; }
    
    // Only checks the password argument
    bool authenticate(const string& inputPassword) const { return password == inputPassword; }
    
    // Virtual serialization for persistence
    virtual string serialize() const = 0;
    virtual ~User() {}
};

// --- NEW CLASS 9: Admin (Derived from User) ---
class Admin : public User {
public:
    Admin(const string& u, const string& p) : User(u, p, "Admin") {}
    
    void displayMenu() const override {
        cout << "\n==============================================" << endl;
        cout << "🔒 **Admin Menu** 🔒" << endl;
        cout << "==============================================" << endl;
        cout << "1. View All Trains (Basic Details)" << endl;
        cout << "2. View Train Availability by Date" << endl;
        cout << "3. **Add New Express Train**" << endl;
        cout << "4. Remove Train" << endl;
        cout << "5. View All Bookings (Placeholder)" << endl;
        cout << "6. **Switch User**" << endl; // Updated option
        cout << "7. Exit System" << endl;
        cout << "----------------------------------------------" << endl;
        cout << "Enter your choice: ";
    }
    
    string serialize() const override {
        return "Admin|" + username + "|" + password;
    }
};

// --- NEW CLASS 10: Customer (Derived from User) ---
class Customer : public User {
public:
    Customer(const string& u, const string& p) : User(u, p, "Customer") {}
    
    void displayMenu() const override {
        cout << "\n==============================================" << endl;
        cout << "🎫 **User Menu** 🎫" << endl;
        cout << "==============================================" << endl;
        cout << "1. Search Trains by Route" << endl;
        cout << "2. **Book a Ticket**" << endl;
        cout << "3. View Booking by PNR" << endl;
        cout << "4. **Cancel Booking**" << endl;
        cout << "5. **Switch User**" << endl; // Updated option
        cout << "6. Exit System" << endl;
        cout << "----------------------------------------------" << endl;
        cout << "Enter your choice: ";
    }
    
    string serialize() const override {
        return "Customer|" + username + "|" + password;
    }
};

// --- 11. RailwayManager Class (Singleton/System) ---
// Handles all data management, persistence, and core logic.
class RailwayManager {
private:
    vector<Train*> trains; 
    vector<Booking> bookings; 
    vector<User*> users; // Stores User objects (Admin/Customer)
    PNRGenerator pnrGenerator; // Using the dedicated PNRGenerator class

    // Private Constructor for Singleton
    RailwayManager() {
        loadData(); 
    }
    
    // Train lookup helper
    Train* findTrain(const string& tNum) {
        for (auto& train : trains) {
            if (train->getTrainNumber() == tNum) {
                return train;
            }
        }
        return nullptr;
    }

    // --- File Persistence Implementation ---
    void saveUsers() const {
        ofstream userFile(USER_FILE);
        if (userFile.is_open()) {
            for (const auto& user : users) {
                userFile << user->serialize() << "\n";
            }
            userFile.close();
        }
    }

    User* deserializeUser(const string& line) {
        stringstream ss(line);
        string segment;
        vector<string> parts;
        while (getline(ss, segment, '|')) {
            parts.push_back(segment);
        }
        if (parts.size() == 3) {
            if (parts[0] == "Admin") return new Admin(parts[1], parts[2]);
            if (parts[0] == "Customer") return new Customer(parts[1], parts[2]);
        }
        return nullptr;
    }

    void loadUsers() {
        ifstream userFile(USER_FILE);
        string line;
        while (getline(userFile, line)) {
            if (!line.empty()) {
                User* user = deserializeUser(line);
                if (user) users.push_back(user);
            }
        }
        // Add initial dummy data if files are empty
        if (users.empty()) {
            users.push_back(new Admin("admin", "123"));
            users.push_back(new Customer("user", "123"));
        }
    }
    
    void saveData() const {
        // Save Train data
        ofstream trainFile(TRAIN_FILE);
        if (trainFile.is_open()) {
            for (const auto& train : trains) {
                trainFile << train->serialize() << "\n";
            }
            trainFile.close();
        }

        // Save Booking data
        ofstream bookingFile(BOOKING_FILE);
        if (bookingFile.is_open()) {
            for (const auto& booking : bookings) {
                bookingFile << booking.serialize() << "\n";
            }
            bookingFile.close();
        }
        saveUsers(); // Save users
    }

    void loadData() {
        // Load Train data (Simplified, only loads ExpressTrain)
        ifstream trainFile(TRAIN_FILE);
        string line;
        while (getline(trainFile, line)) {
            if (line.empty()) continue;
            
            stringstream ss(line);
            string type_str;
            getline(ss, type_str, '|');

            if (type_str == "EXPRESS") {
                stringstream ts(line);
                string segment;
                vector<string> parts;
                while (getline(ts, segment, '|')) {
                    parts.push_back(segment);
                }
                
                if (parts.size() >= 8) { // Check size
                    try {
                        Route r = Route::deserialize(parts[3]);
                        Train* t = new ExpressTrain(
                            parts[1], parts[2], r, stoi(parts[4]), stod(parts[5]), (parts[6] == "1")
                        );
                        
                        size_t seatmap_start = line.find(parts[7]); 
                        if (seatmap_start != string::npos) {
                            string seatmap_data = line.substr(seatmap_start);
                            dynamic_cast<ExpressTrain*>(t)->deserializeSeatMap(seatmap_data); 
                        }
                        trains.push_back(t);
                    } catch (const std::exception& e) {
                        cerr << "[Error] Train deserialization failed: " << e.what() << ". Skipping record: " << line.substr(0, 30) << "..." << endl;
                    }
                }
            }
        }
        
        // Load Booking data
        ifstream bookingFile(BOOKING_FILE);
        while (getline(bookingFile, line)) {
            if (!line.empty()) {
                bookings.push_back(Booking::deserialize(line));
            }
        }
        
        loadUsers(); // Load users
        
        // Add initial dummy data if files are empty
        if (trains.empty()) {
            trains.push_back(new ExpressTrain("ET001", "Fast Express", Route("CityA", "CityB"), 100, 55.00, true));
            trains.push_back(new ExpressTrain("SR205", "Slow Runner", Route("CityB", "CityC"), 50, 75.50, false));
        }
    }

public:
    // Delete copy constructor and assignment operator for Singleton
    RailwayManager(const RailwayManager&) = delete;
    RailwayManager& operator=(const RailwayManager&) = delete;

    // Static method to get the single instance
    static RailwayManager& getInstance() {
        static RailwayManager instance;
        return instance;
    }
    
    // Authenticates user against stored users
    User* authenticate(const string& username, const string& password) const {
        for (User* user : users) {
            if (user->getUsername() == username) {
                // Only pass the password to the user object's authenticate method
                if (user->authenticate(password)) { 
                    return user;
                }
            }
        }
        return nullptr;
    }

    // --- Core System Features ---

    void addTrain(Train* train) {
        if (findTrain(train->getTrainNumber())) {
            cout << "\n❌ Error: Train number already exists." << endl;
            delete train; 
            return;
        }
        trains.push_back(train);
        cout << "\n✅ New Train **" << train->getTrainNumber() << "** added successfully." << endl;
        saveData(); 
    }
    
    // (Admin Feature)
    bool removeTrain(const string& tNum) {
        auto it = remove_if(trains.begin(), trains.end(), 
                            [&tNum](Train* t){ return t->getTrainNumber() == tNum; });
        if (it != trains.end()) {
            delete *it; 
            trains.erase(it, trains.end());
            saveData(); 
            cout << "\n✅ Train **" << tNum << "** removed successfully." << endl;
            return true;
        }
        cout << "\n❌ Error: Train **" << tNum << "** not found." << endl;
        return false;
    }

    void viewAllTrains(const string& date = "") const {
        cout << "\n## Available Trains" << (date.empty() ? "" : " for " + date) << " ##" << endl;
        if (trains.empty()) {
            cout << "No trains currently available." << endl;
            return;
        }
        for (const auto& train : trains) {
            train->displayDetails(); 
            if (!date.empty()) {
                Train* nonConstTrain = const_cast<RailwayManager*>(this)->findTrain(train->getTrainNumber());
                int available = nonConstTrain ? nonConstTrain->getAvailableSeats(date) : -1;
                
                if (available >= 0) {
                     cout << "    Available Seats on " << date << ": **" << available << "**" << endl;
                }
            }
            cout << "----------------------" << endl;
        }
    }
    
    void searchTrain(const string& src, const string& dest, const string& date) {
        cout << "\n## Search Results (" << src << " to " << dest << " on " << date << ") ##" << endl;
        bool found = false;
        for (auto& train : trains) {
            if (train->getSource() == src && train->getDestination() == dest) {
                train->displayDetails();
                int available = train->getAvailableSeats(date);
                if (available >= 0) {
                    cout << "    Available Seats on " << date << ": **" << available << "**" << endl;
                }
                cout << "----------------------" << endl;
                found = true;
            }
        }
        if (!found) {
            cout << "No direct trains found from **" << src << "** to **" << dest << "**." << endl;
        }
    }

    void bookTicket(const string& tNum, const string& date, int numPassengers) {
        Train* selectedTrain = findTrain(tNum);

        if (!selectedTrain) {
            cout << "\n❌ Booking failed. Train **" << tNum << "** not found." << endl;
            return;
        }
        
        if (selectedTrain->bookSeat(date, numPassengers)) {
            vector<Passenger> newPassengers;
            
            // Collect passenger details
            for (int i = 0; i < numPassengers; ++i) {
                string name, gender;
                int age;
                cout << "    --- Passenger " << (i+1) << " Details ---" << endl;
                cout << "    Name: "; cin.ignore(); getline(cin, name);
                cout << "    Age: "; 
                while (!(cin >> age) || age <= 0 || age >= 120) {
                    cout << "    Invalid age (1-120). Please re-enter: ";
                    clearInputBuffer();
                }
                cout << "    Gender (M/F/O): "; cin >> gender;
                newPassengers.emplace_back(name, age, gender);
            }
            
            double fare = selectedTrain->getBaseFare() * numPassengers;
            string pnr = pnrGenerator.generate(); // Use the dedicated PNR generator
            
            Booking newBooking(pnr, tNum, date, newPassengers, fare);
            bookings.push_back(newBooking);
            
            cout << "\n✅ **Booking successful!** PNR: **" << pnr << "**" << endl;
            newBooking.displayBooking();
            saveData();
        } else {
            cout << "\n❌ Booking failed. Only **" << selectedTrain->getAvailableSeats(date) << "** seats available on " << date << "." << endl;
        }
    }

    void cancelBooking(const string& pnr) {
        auto it = find_if(bookings.begin(), bookings.end(), 
                          [&pnr](const Booking& b){ return b.getPNR() == pnr; });
        
        if (it != bookings.end()) {
            if (it->getStatus() == "Confirmed") {
                Train* selectedTrain = findTrain(it->getTrainNumber());

                if (selectedTrain) {
                    selectedTrain->cancelSeat(it->getDate(), it->getNumPassengers());
                    it->setStatus("Cancelled");
                    cout << "\n✅ **Cancellation successful** for PNR: **" << pnr << "**" << endl;
                    cout << "    Refund amount: ₹" << it->getTotalFare() << endl;
                    saveData();
                } else {
                    cout << "\n❌ Cancellation failed. Associated Train not found." << endl;
                }
            } else {
                 cout << "\n❌ Booking " << pnr << " is already **" << it->getStatus() << "**." << endl;
            }
        } else {
            cout << "\n❌ Cancellation failed. PNR **" << pnr << "** not found." << endl;
        }
    }

    void viewBookingByPNR(const string& pnr) const {
        auto it = find_if(bookings.begin(), bookings.end(), 
                          [&pnr](const Booking& b){ return b.getPNR() == pnr; });
        
        if (it != bookings.end()) {
            it->displayBooking();
        } else {
            cout << "\n❌ PNR **" << pnr << "** not found." << endl;
        }
    }
    
    // Destructor to clean up dynamically allocated Train objects and Users
    ~RailwayManager() {
        for (auto train : trains) {
            delete train;
        }
        for (auto user : users) {
            delete user;
        }
    }
};

// --- 12. Main Entry Point ---

void handleAddTrain(RailwayManager& manager) {
    cout << "\n--- Add New Express Train ---" << endl;
    string num, name, src, dest;
    int seats;
    double fare;
    string pantry;
    bool hasPantry;
    
    cout << "Enter Train Number (e.g., ET003): "; cin >> num;
    cout << "Enter Train Name: "; cin.ignore(); getline(cin, name);
    cout << "Enter Source Station: "; cin >> src;
    cout << "Enter Destination Station: "; cin >> dest;
    
    cout << "Enter Total Seats: "; 
    while (!(cin >> seats) || seats <= 0) {
        cout << "    Invalid seat count. Please re-enter: ";
        clearInputBuffer();
    }
    
    cout << "Enter Base Fare: ₹"; 
    while (!(cin >> fare) || fare <= 0) {
        cout << "    Invalid fare. Please re-enter: ";
        clearInputBuffer();
    }
    
    cout << "Has Pantry Car (yes/no)? "; cin >> pantry;
    hasPantry = (pantry == "yes" || pantry == "Yes" || pantry == "y" || pantry == "Y");
    
    Route r(src, dest);
    Train* newTrain = new ExpressTrain(num, name, r, seats, fare, hasPantry);
    manager.addTrain(newTrain);
}

void runUserActions(RailwayManager& manager, bool& running, User* currentUser, bool& shouldSwitch) {
    int choice;
    string tempStr1, tempStr2, tempStr3;
    int tempInt;
    
    currentUser->displayMenu();
    if (!(cin >> choice)) { clearInputBuffer(); return; }

    switch (choice) {
        case 1: // Search Trains
            cout << "Enter Source Station: "; cin >> tempStr1;
            cout << "Enter Destination Station: "; cin >> tempStr2;
            cout << "Enter Date of Journey (MM/DD/YYYY): "; cin >> tempStr3;
            if (!isValidDate(tempStr3)) { cout << "❌ Invalid Date Format." << endl; break; }
            manager.searchTrain(tempStr1, tempStr2, tempStr3);
            break;

        case 2: // Book Ticket
            cout << "Enter Train Number to book: "; cin >> tempStr1;
            cout << "Enter Date of Journey (MM/DD/YYYY): "; cin >> tempStr2;
            if (!isValidDate(tempStr2)) { cout << "❌ Invalid Date Format." << endl; break; }
            cout << "Enter Number of Passengers (max 6): "; 
            if (!(cin >> tempInt) || tempInt <= 0 || tempInt > 6) {
                cout << "\n❌ Invalid number of passengers (1-6 allowed)." << endl;
                clearInputBuffer();
                break;
            }
            manager.bookTicket(tempStr1, tempStr2, tempInt);
            break;
            
        case 3: // View Booking
            cout << "Enter PNR to view booking: "; cin >> tempStr1;
            manager.viewBookingByPNR(tempStr1);
            break;

        case 4: // Cancel Booking
            cout << "Enter PNR to cancel booking: "; cin >> tempStr1;
            manager.cancelBooking(tempStr1);
            break;
        
        case 5: // Switch User
            shouldSwitch = true;
            cout << "\n➡️ Switching user..." << endl;
            break;
            
        case 6: // Exit System
            running = false;
            cout << "\n👋 Thank you for using the Railway Management System. Goodbye!" << endl;
            break;

        default:
            cout << "\n⚠️ Invalid choice. Please try again (1-6)." << endl;
            break;
    }
}

void runAdminActions(RailwayManager& manager, bool& running, User* currentUser, bool& shouldSwitch) {
    int choice;
    string tempStr1, tempStr2;
    
    currentUser->displayMenu();
    if (!(cin >> choice)) { clearInputBuffer(); return; }

    switch (choice) {
        case 1: // View All Trains
            manager.viewAllTrains();
            break;

        case 2: // View Availability by Date
            cout << "Enter Date (MM/DD/YYYY) to check availability: "; cin >> tempStr1;
            if (!isValidDate(tempStr1)) { cout << "❌ Invalid Date Format." << endl; break; }
            manager.viewAllTrains(tempStr1);
            break;
        
        case 3: // Add New Train
            handleAddTrain(manager);
            break;

        case 4: // Remove Train
            cout << "Enter Train Number to remove: "; cin >> tempStr1;
            manager.removeTrain(tempStr1);
            break;
            
        case 5: // View All Bookings (Placeholder)
            cout << "\n⚠️ Feature not fully implemented. Reviewing all bookings requires a dedicated admin view." << endl;
            break;

        case 6: // Switch User
            shouldSwitch = true;
            cout << "\n➡️ Switching user..." << endl;
            break;
            
        case 7: // Exit System
            running = false;
            cout << "\n👋 Thank you for using the Railway Management System. Goodbye!" << endl;
            break;

        default:
            cout << "\n⚠️ Invalid choice. Please try again (1-7)." << endl;
            break;
    }
}


int main() {
    // Get the singleton instance of the manager
    RailwayManager& manager = RailwayManager::getInstance();
    
    bool systemRunning = true;
    User* currentUser = nullptr;

    // Outer Loop for the entire program duration
    while (systemRunning) {
        bool sessionActive = false;
        
        // Authentication Loop
        while (currentUser == nullptr && systemRunning) {
            cout << "\n==============================================" << endl;
            cout << "🚂 **Railway System Login** 🔒" << endl;
            cout << "==============================================" << endl;
            cout << " (Try: admin/123 or user/123)" << endl;
            cout << "Enter 'quit' for Username to exit system." << endl;
            string uname, pword;
            cout << "Username: "; cin >> uname;
            if (uname == "quit") {
                systemRunning = false;
                break;
            }
            cout << "Password: "; cin >> pword;
            
            currentUser = manager.authenticate(uname, pword);
            
            if (currentUser == nullptr) {
                cout << "\n❌ Login failed. Invalid credentials. Press ENTER to try again.";
                clearInputBuffer();
                cin.get(); 
            } else {
                cout << "\n✅ Welcome, " << currentUser->getUsername() << " (" << currentUser->getRole() << ")!" << endl;
                sessionActive = true;
            }
        }

        // Session Loop (Runs only if logged in)
        while (sessionActive) {
            bool shouldSwitch = false;
            
            if (currentUser->getRole() == "Admin") {
                runAdminActions(manager, systemRunning, currentUser, shouldSwitch);
            } else {
                runUserActions(manager, systemRunning, currentUser, shouldSwitch);
            }

            if (shouldSwitch) {
                currentUser = nullptr; // Clear current user to restart login loop
                sessionActive = false;
                continue;
            }
            
            if (!systemRunning) {
                sessionActive = false; // Exit outer loop
            }

            if (sessionActive) {
                cout << "\nPress ENTER to continue...";
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cin.get();
            }
        }
    }
    
    cout << "\n👋 System Shut Down. Data saved successfully." << endl;
    return 0;
}