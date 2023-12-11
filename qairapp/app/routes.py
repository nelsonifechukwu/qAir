from app import app 
from flask import session, make_response, redirect, render_template, request, flash, url_for, Response
from app.models import checklogin, update_sms_state, get_sms_state, delete_poll_data, insert_pollution_data, get_pollution_data, get_poll_state, update_poll_state
import json, random

API = 'xdol'

#login
@app.route('/', methods = ('GET', 'POST'))
@app.route('/login' , methods = ('GET', 'POST'))
def login():
    if (request.method=='POST'):
        username = request.form['username']
        password = request.form['password']

        succeed = checklogin(username, password)
        if (succeed):
            session['username'] = username
            
            flash('Welcome! Login Successful')
            return redirect(url_for('plot'))
        else:
            flash('Error: Invalid Username or Password')
            return redirect(url_for('login'))
    else:
        return render_template('login.html')

#plot data
@app.route('/plot')
def plot():
    #create a session to secure this endpoint
    if "username" not in session:
        return redirect(url_for('login'))
    #data = get_transformer_data()
    return render_template('plot.html', name=session["username"])


#design the web_api
@app.route('/update/key=<api_key>/c1=<int:c1>/c2=<int:c2>/c3=<int:c3>/c4=<int:c4>/c5=<int:c5>', methods=['GET'])
def update(api_key, c1, c2, c3, c4, c5):
    if (api_key == API):
        insert_pollution_data(c1, c2, c3, c4, c5)
        return Response(status=200)
    else:
        return Response(status=500)

#fetch data to be plotted
@app.route('/data', methods=['GET', 'POST'])
def get_data():
    data = get_pollution_data()
    data = str(data)
    data = data.strip('[]()')
    return str(data)

#get button state (for the hardware)
@app.route('/get_state', methods = ['GET'])
def get_state():
    poll_state = get_poll_state('all')
    poll_state = str(poll_state)
    poll_state = poll_state.strip('[]()')
    return str(poll_state)

@app.route('/sms_state', methods = ['GET', 'POST'])
def get_sms_status():
    sms_state = get_sms_state()
    sms_state = str(sms_state)
    sms_state = sms_state.strip('[]()')
    return str(sms_state)

@app.route('/update/<sensor>/<state>', methods = ['GET', 'POST'])
def update_sms(sensor, state):
    update_sms_state(sensor, state)
    return Response(status=200)

#update button state
@app.route('/updatestate/button=<id>', methods = ['GET', 'POST'])
def update_state(id):
    #get the previous state
    state = get_poll_state(id)
    state =  str(state)
    #Strip the string from the database and update the previous state
    state = state.strip('()[]').strip(',')
    newstate =  1-int(state)
    update_poll_state(newstate, id)
    return str(newstate)

#delete all data
@app.route('/delete', methods = ('GET', 'POST'))
def delete():
    if request.method == 'POST':
        delete_poll_data()
        insert_pollution_data(0,0,0,0,0)
        return redirect(url_for('plot'))

#get individual state
@app.route('/get_state/<id>', methods = ['GET'])
def get_id_state(id):
    poll_state = get_poll_state(id)
    poll_state = str(poll_state)
    poll_state = poll_state.strip('[]()')
    return str(poll_state)

#logout of session
@app.route('/logout', methods=['POST'])
def log_out():
    session.pop("username")
    return redirect(url_for('login'))