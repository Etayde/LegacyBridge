import csv
import time
import random
from datetime import datetime

# Define a menu of items with their names, categories, and prices (ILS).
MENU = [
    {"name": "Steak Tartare", "category": "Food", "price": 65},
    {"name": "Bibimbap Bulgogi", "category": "Food", "price": 72},
    {"name": "Beef Stir-fry", "category": "Food", "price": 68},
    {"name": "Shakshuka", "category": "Food", "price": 48},
    {"name": "Hummus Plate", "category": "Food", "price": 35},
    {"name": "Goldstar Beer", "category": "Alcohol", "price": 28},
    {"name": "Arak", "category": "Alcohol", "price": 22},
    {"name": "Cola", "category": "Beverage", "price": 14}
]

# The log file where all mock POS transactions will be recorded.
FILE_NAME = "mock_pos.csv"

def generate_mock_data():
    """Continuously generates random point-of-sale (POS) orders and appends them to a CSV file."""
    
    # Initialize the CSV file and write the header row.
    with open(FILE_NAME, mode='w', newline='', encoding='utf-8') as file:
        writer = csv.writer(file)
        writer.writerow(["Timestamp", "Table_ID", "Item_Name", "Category", "Price"])

    print(f"Started writing mock POS data to {FILE_NAME}...")
    print("Press Ctrl+C to stop.\n")

    try:
        while True:
            with open(FILE_NAME, mode='a', newline='', encoding='utf-8') as file:
                writer = csv.writer(file)
                
                # Randomly pick an item and a table number (assuming 15 tables).
                item = random.choice(MENU)
                table_id = random.randint(1, 15)
                
                # Get the current timestamp.
                timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                
                # Write the simulated order directly to the CSV file.
                writer.writerow([timestamp, table_id, item["name"], item["category"], item["price"]])
                
                # Flush the file buffer to ensure data is written to disk immediately.
                file.flush() 
                
                # Log the generated order to the console.
                print(f"[{timestamp}] Added order: Table {table_id} ordered {item['name']} ({item['price']} ILS)")
                
            # Pause for a random duration (1-4 seconds) before the next simulated order.
            time.sleep(random.randint(1, 4))
            
    except KeyboardInterrupt:
        # Allow the user to gracefully stop the script with Ctrl+C.
        print("\nService ended. Stopped generating data.")

if __name__ == "__main__":
    # Start the data generation when the script is run directly.
    generate_mock_data()