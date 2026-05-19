import pandas as pd
import matplotlib.pyplot as plt

# Lit le CSV
df = pd.read_csv("output/simulation_data.csv")
# Plot un graphique
plt.figure(figsize=(10, 6))
colors = ['#000000', '#ff6b62', '#2b5c65', '#4a4a4a'] # Teal, Red/Coral, Dark Grey, Black
plt.stackplot(df['Step'], df['Dead'], df['Infected'], df['Susceptible'], df['Recovered'], colors=colors, labels=['R', 'I', 'D', 'S']) 


plt.xlabel('Time Steps')
plt.ylabel('Agent Population')
plt.title('SIR Model Simulation Output')
plt.margins(0,0)
plt.show()