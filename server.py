from flask import Flask, jsonify,redirect,send_file,render_template,make_response
from flask import request
import pymysql
import datetime
import logging
from datetime import datetime
app = Flask(__name__)

def insert_to_db(device_id,tmp,hmd,now):
    try:
        connection = pymysql.connect(host='127.0.0.1',
            port = 3306,
            user = 'Lora-user',
            password = 'Aas9d$69ash',
            database = 'LoRa',
            cursorclass = pymysql.cursors.DictCursor)
        with connection.cursor() as cursor:
            cursor.execute(f"Insert into real_time_data (id,tmp,hmd,time) values (%s,%s,%s,%s)",(device_id,tmp,hmd,now))
            connection.commit()
            cursor.close()
            connection.close()
    except Exception as ex:
        print(ex)


def get_configs(id):
    try:
        connection = pymysql.connect(host='127.0.0.1',
            port=3306,
            user='Lora-user',
            password='Aas9d$69ash',
            database='LoRa',
            cursorclass=pymysql.cursors.DictCursor)

        with connection.cursor() as cursor:
            cursor.execute(f"select * from configs where id={id}")
            data=cursor.fetchone()
            connection.commit()
            cursor.close()
            connection.close()
        print(data)
        return data

    except Exception as ex:
        print(ex)


def get_from_db():
    try:
        connection = pymysql.connect(host='127.0.0.1',
            port=3306,
            user='Lora-user',
            password='Aas9d$69ash',
            database='LoRa',
            cursorclass=pymysql.cursors.DictCursor)

        with connection.cursor() as cursor:
            cursor.execute(f"select avg(hmd) as hmd from (select hmd from real_time_data order by time desc limit 50) as d")

            data=cursor.fetchone()
            connection.commit()
            cursor.close()
            connection.close()
        hmd_avg=data['hmd']
        print(hmd_avg)
        return round(hmd_avg,3)

    except Exception as ex:
        print(ex)

def flag(type,id,flag):
    if type=="update":
        try:
            now=datetime.now()
            print(now)
            connection = pymysql.connect(host='127.0.0.1',
                port=3306,
                user='Lora-user',
                password='Aas9d$69ash',
                database='LoRa',
                cursorclass=pymysql.cursors.DictCursor)

            with connection.cursor() as cursor:
                cursor.execute(f"update flags set flag=%s,time=%s where id=%s",(flag,now,id))
                connection.commit()
                cursor.close()
                connection.close()

        except Exception as ex:
            print(ex)

    elif type=="get":
        try:
            connection = pymysql.connect(host='127.0.0.1',
                port=3306,
                user='Lora-user',
                password='Aas9d$69ash',
                database='LoRa',
                cursorclass=pymysql.cursors.DictCursor)

            with connection.cursor() as cursor:
                cursor.execute(f"select flag from flags where id={id}")
                data=cursor.fetchone()
                connection.commit()
                cursor.close()
                connection.close()
                print(data)
            return data['flag']

        except Exception as ex:
            print(ex)

def check_last_watering(id,interval):
    try:
        connection = pymysql.connect(host='127.0.0.1',
            port=3306,
            user='Lora-user',
            password='Aas9d$69ash',
            database='LoRa',
            cursorclass=pymysql.cursors.DictCursor)

        with connection.cursor() as cursor:
            cursor.execute(f"SELECT TIMESTAMPDIFF(MINUTE, time, now()) as t from flags where id={id}")
            data=cursor.fetchone()
            connection.commit()
            cursor.close()
            connection.close()
            print(data)
            if data['t']>interval:
                return True
            else:
                return False

    except Exception as ex:
        print(ex)



def water_level():
    try:
        connection = pymysql.connect(host='127.0.0.1',
            port=3306,
            user='Lora-user',
            password='Aas9d$69ash',
            database='LoRa',
            cursorclass=pymysql.cursors.DictCursor)

        with connection.cursor() as cursor:
            cursor.execute(f"select level from water_level order by time desc limit 1")
            data=cursor.fetchone()
            connection.commit()
            cursor.close()
            connection.close()
            print(data)
        return data['level']
    except Exception as ex:
        print(ex)

@app.route('/get', methods=['POST'])
def get():
    data=request.data
    decoded=data.decode("ISO-8859-1")
    print(decoded)
    garden_id=decoded.split('|')[0]

    current_hmd=get_from_db()
    print(current_hmd)
    config_hmd=get_configs(garden_id)

    if current_hmd<config_hmd['top_val'] and current_hmd<config_hmd['bottom_val']:
        v=check_last_watering(garden_id,config_hmd['intrvl'])
        l=water_level()
        if(l<config_hmd['bottom_water_lvl']):
            if(v==True):
                flag("update",garden_id,0)
                return f"yes|{config_hmd['water_opened_interval']}"

            elif(v==False):
               return "no"

    else:
        return "no"


@app.route('/registrate', methods=['POST'])
def index():
    if request.method=='POST':
        try:
            data = request.data
            decoded=data.decode("ISO-8859-1")
            print(decoded)
            if (len(decoded)>36):
                m=decoded.split(" ")
                for i in range(len(m)-1):
                    try:
                        t=decoded.split(" ")[i]
                        print(t)
                        device_id=t.split('|')[0]
                        right_part=t.split('|')[1]

                        tmp=float(right_part.split('&')[0])
                        h=right_part.split('&')[1]
                        hmd=h.split('?')[0]
                        tm=right_part.split('?')[1]

                        datetime_object = datetime.strptime(tm, '%Y/%m/%d_%H:%M:%S')

                        insert_to_db(device_id,tmp,hmd,datetime_object)

                    except Exception as ex:
                        print(ex)



                print(len(m))
            else:
                try:
                    device_id=decoded.split('|')[0]

                    right_part=decoded.split('|')[1]

                    tmp=float(right_part.split('&')[0])
                    h=right_part.split('&')[1]
                    hmd=h.split('?')[0]
                    tm=right_part.split('?')[1]
                    datetime_object = datetime.strptime(tm, '%Y/%m/%d_%H:%M:%S')
                    insert_to_db(device_id,tmp,hmd,datetime_object)

                except Exception as ex:
                    print(ex)

            return '1'
        except Exception as ex:
            print(ex)




if __name__ == "__main__":
    app.run(host ='0.0.0.0', port = 8090, debug =False)