from flask import Flask, render_template, request
import pandas as pd

app = Flask(__name__)

# Load the CSV files
file_paths = {
    'sqlite': 'resultsqlite.csv',
    'mysql': 'resultmsql.csv',
    'postgres': 'resultpostgres1.csv',
    'redis': 'resultredis.csv'
}

# Function to load and process data
def load_data(file_path):
    data = pd.read_csv(file_path)
    data = data[data['name'].str.startswith('/db/')].copy()
    data['latency'] = data['elapsed'] / data['operations'] * 1000  # Latency in milliseconds
    return data

# Load data for all databases
db_data = {db: load_data(path) for db, path in file_paths.items()}

# Function to recommend database
def recommend_database(num_files, operation_type):
    scores = {}

    for db, data in db_data.items():
        # Filter data based on operation type
        op_data = data[data['name'] == f'/db/{operation_type}']
        if op_data.empty:
            continue

        # Calculate score based on throughput and latency
        throughput = op_data['operations'].mean()
        latency = op_data['latency'].mean()

        # Example scoring: prioritize high throughput and low latency
        score = (throughput / latency) * 100  # Adjust weight as needed
        scores[db] = score

    # Recommend the database with the highest score
    recommended_db = max(scores, key=scores.get) if scores else None
    return recommended_db

@app.route('/', methods=['GET', 'POST'])
def index():
    if request.method == 'POST':
        num_files = int(request.form['num_files'])
        operation_type = request.form['operation_type']
        recommended_db = recommend_database(num_files, operation_type)
        return render_template('index.html', recommendation=recommended_db)

    return render_template('index.html', recommendation=None)

if __name__ == '__main__':
    app.run(debug=True)
