package main
import "fmt"

func main() {
	fmt.Println(`     
	<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Basic HTML Page</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 0;
            display: flex;
            flex-direction: column;
            align-items: center;
        }
        header, footer {
            background-color: #f8f9fa;
            padding: 10px;
            width: 100%;
            text-align: center;
        }
        main {
            padding: 20px;
            max-width: 800px;
        }
    </style>
</head>
<body>
    <header>
        <h1>Welcome to My Website</h1>
    </header>
    <main>
        <h2>About Me</h2>
        <p>Hi, I'm [Your Name]. This is a basic HTML page example.</p>
        <h2>Hobbies</h2>
        <ul>
            <li>Reading</li>
            <li>Traveling</li>
            <li>Coding</li>
        </ul>
        <h2>Contact</h2>
        <p> You can reach me at <a href="mailto:your.email@example.com">your.email@example.com</a></p>
    </main>
    <footer>
        <p>&copy; 2024 [Your Name]</p>
    </footer>
</body>
</html>
`)
}
