import sqlite3 as sql
from datetime import datetime

def checklogin(username, password):
    con = sql.connect("qair.db")
    cur = con.cursor()
    cur.execute("SELECT username, password FROM users")
    users = cur.fetchall()

    if username == users[0][0] and password == users[0][1]:
        success = True
    else:
        success = False
    con.commit()
    con.commit()
    con.close()
    return success

def insert_pollution_data(c1, c2, c3, c4, c5):
    con = sql.connect("qair.db")
    cur = con.cursor()
    #add datetime to the query
    cur.execute("INSERT INTO pollution_data(c1, c2, c3, c4, c5) VALUES (?,?,?,?,?)", (c1, c2, c3, c4, c5))# datetime.now()
    con.commit()
    con.close()

def get_pollution_data():
    con = sql.connect("qair.db")
    cur = con.cursor()
    #get the latest data
    cur.execute("SELECT * FROM pollution_data ORDER BY id DESC LIMIT 40")
    poll_data = cur.fetchall()
    con.commit()
    con.close()
    return poll_data

def get_sms_state():
    con = sql.connect("qair.db")
    cur = con.cursor()
    cur.execute("SELECT * FROM sms_state")
    sms_data = cur.fetchall()
    con.commit()
    con.close()
    return sms_data

def update_sms_state(sensor, state):
    con = sql.connect("qair.db")
    cur = con.cursor()
    query = f"UPDATE sms_state SET sms_{sensor} = {state} WHERE id = 1"
    
    with sql.connect("qair.db") as con:
        cur = con.cursor()
        cur.execute(query)
        con.commit()


def get_poll_state(type):
    con = sql.connect("qair.db")
    cur = con.cursor()

    if type == 'all':
        cur.execute("SELECT c1_state, c2_state, c3_state, c4_state, c5_state FROM poll_state WHERE id = 1")
    else:
        cur.execute("SELECT {}_state FROM poll_state WHERE id = 1".format(type))

    poll_state = cur.fetchall()
    con.commit()
    con.close()
    return poll_state

def update_poll_state(state, type):
    column_name = f"{type}_state"
    value = int(state)
    query = f"UPDATE poll_state SET {column_name} = {value} WHERE id = 1"

    with sql.connect("qair.db") as con:
        cur = con.cursor()
        cur.execute(query)
        con.commit()

def delete_poll_data():
    con = sql.connect("qair.db")
    cur = con.cursor()
    cur.execute("DELETE FROM pollution_data")
    con.commit()
    con.close()

