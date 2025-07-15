from fastapi import FastAPI
from fastapi.responses import HTMLResponse

app = FastAPI()


@app.get("/")
async def handle_home():
    resp = "<h1>hello from fastapi!!</h1>"
    return HTMLResponse(content=resp)
