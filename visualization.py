import matplotlib.pyplot as plt
import pandas as pd

# Load the CSV files
file_path1 = 'sqlite.csv'  # SQLite
file_path2 = 'mysql.csv'    # MySQL
file_path3 = 'result-postgres.csv'  # PostgreSQL
file_path4 = 'resultrdis.csv'   # Redis

data1 = pd.read_csv(file_path1)
data2 = pd.read_csv(file_path2)
data3 = pd.read_csv(file_path3)
data4 = pd.read_csv(file_path4)

# Filter to include only operations starting with '/db/'
db_data1 = data1[data1['name'].str.startswith('/db/')].copy()
db_data2 = data2[data2['name'].str.startswith('/db/')].copy()
db_data3 = data3[data3['name'].str.startswith('/db/')].copy()
db_data4 = data4[data4['name'].str.startswith('/db/')].copy()

# Calculate Latency for each operation
db_data1['latency'] = db_data1['elapsed'] / db_data1['operations'] * 1000  # Latency in milliseconds
db_data2['latency'] = db_data2['elapsed'] / db_data2['operations'] * 1000  # Latency in milliseconds
db_data3['latency'] = db_data3['elapsed'] / db_data3['operations'] * 1000  # Latency in milliseconds
db_data4['latency'] = db_data4['elapsed'] / db_data4['operations'] * 1000  # Latency in milliseconds

# Sorting data by operation name for better visualization
db_data1_sorted = db_data1.sort_values('name')
db_data2_sorted = db_data2.sort_values('name')
db_data3_sorted = db_data3.sort_values('name')
db_data4_sorted = db_data4.sort_values('name')

# Function to add labels to bars
def add_labels(bars):
    for bar in bars:
        yval = bar.get_height()
        bar_x = bar.get_x() + bar.get_width() / 2
        plt.text(bar_x, yval, f'{yval:.1f}', ha='center', va='bottom', fontsize=8, color='black')

# Creating the plot for Throughput (combined comparison)
fig, ax1 = plt.subplots(figsize=(15, 8))
width = 0.2  # Width of each bar
x = range(len(db_data1_sorted['name']))

# Plotting bars for four datasets side by side
bars1 = ax1.bar(x, db_data1_sorted['operations'], width, color='tab:blue', alpha=0.6, label='sqlite')
bars2 = ax1.bar([i + width for i in x], db_data2_sorted['operations'], width, color='tab:green', alpha=0.6, label='mysql')
bars3 = ax1.bar([i + 2*width for i in x], db_data3_sorted['operations'], width, color='tab:orange', alpha=0.6, label='postgres')
bars4 = ax1.bar([i + 3*width for i in x], db_data4_sorted['operations'], width, color='tab:red', alpha=0.6, label='redis')

ax1.set_xlabel('Operation')
ax1.set_ylabel('Throughput (ops/sec)', color='tab:blue')
ax1.tick_params(axis='y', labelcolor='tab:blue')
ax1.set_xticks([i + 1.5*width for i in x])
ax1.set_xticklabels(db_data1_sorted['name'], rotation='vertical', fontsize=8)
ax1.grid(True, which='both', linestyle='--', linewidth=0.5, axis='y')  # Adding grid lines for better readability

# Add labels to all sets of bars
add_labels(bars1)
add_labels(bars2)
add_labels(bars3)
add_labels(bars4)

plt.legend()
plt.title('Throughput for Database Operations')
plt.tight_layout()
plt.savefig('throughput1.png')

# Creating the plot for Latency (combined comparison)
fig, ax2 = plt.subplots(figsize=(18, 10))
bars1 = ax2.bar(x, db_data1_sorted['latency'], width, color='tab:blue', alpha=0.6, label='sqlite')
bars2 = ax2.bar([i + width for i in x], db_data2_sorted['latency'], width, color='tab:green', alpha=0.6, label='mysql')
bars3 = ax2.bar([i + 2*width for i in x], db_data3_sorted['latency'], width, color='tab:orange', alpha=0.6, label='postgres')
bars4 = ax2.bar([i + 3*width for i in x], db_data4_sorted['latency'], width, color='tab:red', alpha=0.6, label='redis')

ax2.set_xlabel('Operation')
ax2.set_ylabel('Latency (ms/op)', color='tab:red')
ax2.tick_params(axis='y', labelcolor='tab:red')
ax2.set_xticks([i + 1.5*width for i in x])
ax2.set_xticklabels(db_data1_sorted['name'], rotation='vertical', fontsize=8)
ax2.grid(True, which='both', linestyle='--', linewidth=0.5, axis='y')  # Adding grid lines for better readability

# Add labels to all sets of bars
add_labels(bars1)
add_labels(bars2)
add_labels(bars3)
add_labels(bars4)

plt.legend()
plt.title('Latency for Database Operations')
plt.tight_layout()
plt.savefig('latency1.png')
